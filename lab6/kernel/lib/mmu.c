#include "mmu.h"
#include "malloc.h"
#include "sched.h"
#include "string.h"
#include "irq.h"
// PGD's page frame at 0x1000
// PUD's page frame at 0x2000
// PMD1's page frame at 0x3000
// PMD2's page frame at 0x4000
// two-level translation (1G) -> three-level translation (2MB)

void *set_2M_kernel_mmu(void *x0)
{
    // setting up first PMD which maps to 0x0000000 ~ 0x3fffffff 

    unsigned long *PMD = (unsigned long *)0x3000;
    // 512 entries per page table
    // 0x80000000 / 2MB = 1024 = 512 * 2, -> 2 PMD
    for (int i = 0; i < 512; i++)
    {
        // 0x00000000 ~ 0x3F000000 normal RAM -> without cache
        // normal RAM doesnt need PD_UKAccess or UNX or KNX
        // gathering, reordering , speculative execution are possible
        // if RAM set them, then el0 el1 cannot execute 
        // 0x3F000000 to 0x3FFFFFFF for GPU peripherals -> Device memory nGnRnE
        // nG no gathering : memory access cannot merge
        // nR no Reordering : strictly accroding to program order
        // nE transaction ack must from end point
        // every 2MB -> 3 level
        if ((0x00000000 + (0x200000L) * i) >= 0x3F000000L)
            PMD[i] = PD_ACCESS | PD_BLOCK | (0x00000000 + (0x200000L) * i) | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_UK_ACCESS | PD_UNX | PD_KNX;
        else
            PMD[i] = PD_ACCESS | PD_BLOCK | (0x00000000 + (0x200000L) * i) | (MAIR_IDX_NORMAL_NOCACHE << 2);
        // 0x3c0000000 ~ 0x3f0000000 is GPU (mailbox ..etc) memory
    }

    // setting up second PMD which maps to  0x40000000 ~ 0x7fffffff
    unsigned long *PMD2 = (unsigned long *)0x4000;
    for (int i = 0; i < 512; i++)
        // 0x40000000 ~ 0x7fffffff ARM local peripherals -> Device memory nGnRnE
        PMD2[i] = PD_ACCESS | PD_BLOCK | (0x40000000 + (0x200000L) * i) | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_UK_ACCESS | PD_UNX | PD_KNX;

    // PUD first entry point to first PMD
    *(unsigned long *)(PUD) = PD_ACCESS | PD_TABLE | (unsigned long)PMD;
    // PUD second entry point to second PMD
    *(unsigned long *)(PUD + 0x8) = PD_ACCESS | PD_TABLE | (unsigned long)PMD2;

    return x0;
}

// pa,va aligned to 4K
void map_one_page(size_t *virt_pgd_p, size_t va, size_t pa, size_t flag)
{
    size_t *table_p = virt_pgd_p;
    // 4 level page table
    for (int level = 0; level < 4; level++)
    {
        //get table index according descriptor
        // 0x1ff first 9 bits
        // shift right va to get to bit corresponding to current table
        unsigned int idx = (va >> (39 - level * 9)) & 0x1ff;
        // page level
        if (level == 3)
        {   
            // setting page
            table_p[idx] = pa;
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KNX | flag;
            return;
        }

        if (!table_p[idx])
        {
            size_t *newtable_p = malloc(0x1000);
            memset(newtable_p, 0, 0x1000);
            // arm walk get virtual address
            // change virtual newtable_p to physical
            table_p[idx] = VIRT_TO_PHYS((size_t)newtable_p);
            // normal memory no cache
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2);
        }
        // page table is set on linear mapped kernel space
        table_p = (size_t *)PHYS_TO_VIRT((size_t)(table_p[idx] & ENTRY_ADDR_MASK));
    }
}

void add_vma(thread_t *t, size_t va, size_t size, size_t pa, size_t rwx, int is_alloced)
{
    // alignment 4k
    size = size % 0x1000 ? size + (0x1000 - size % 0x1000) : size;
    // malloc this page
    // but the page table of user doesnt have an entry corresponding this page
    vm_area_struct_t *new_area = malloc(sizeof(vm_area_struct_t));
    new_area->rwx = rwx;
    new_area->area_size = size;
    new_area->virt_addr = va;
    new_area->phys_addr = pa;
    new_area->is_alloced = is_alloced;
    // memorize this vma, demand paging -> when use this vma, map to memory space
    list_add_tail((list_head_t *)new_area, &t->vma_list);
}

void free_page_tables(size_t *page_table, int level)
{
    size_t *table_virt = (size_t *)PHYS_TO_VIRT((char *)page_table);
    // traverse page tables
    for (int i = 0; i < 512; i++)
    {
        if (table_virt[i] != 0)
        {
            size_t *next_table = (size_t *)(table_virt[i] & ENTRY_ADDR_MASK);
            if (table_virt[i] & PD_TABLE)
            {
                // recursion free
                if (level != 2)
                    free_page_tables(next_table, level + 1);
                table_virt[i] = 0L;
                free(PHYS_TO_VIRT((char *)next_table));
            }
        }
    }
}

void handle_abort(esr_el1_t *esr_el1)
{
    /*
    Holds the faulting Virtual Address for all synchronous Instruction or Data Abort, 
    PC alignment fault and Watchpoint exceptions that are taken to EL1.
    get current fault vma
    */
    unsigned long long far_el1;
    __asm__ __volatile__("mrs %0, FAR_EL1\n\t"
                         : "=r"(far_el1));

    // check this page fault page is in vma or not
    list_head_t *pos;
    vm_area_struct_t *the_area_ptr = 0;
    list_for_each(pos, &curr_thread->vma_list)
    {
        vm_area_struct_t *vma_tmp = (vm_area_struct_t *)pos;
        if (vma_tmp->virt_addr <= far_el1 && (vma_tmp->virt_addr + vma_tmp->area_size) >= far_el1)
        {
            the_area_ptr = vma_tmp;
            break;
        }
    }

    // if not in vma -> seg_fault
    if (!the_area_ptr)
        seg_fault();

    // is in vma and iss is translation fault
    // -> map this page
    if ((esr_el1->iss & 0x3f) == TF_LEVEL0 || (esr_el1->iss & 0x3f) == TF_LEVEL1 || (esr_el1->iss & 0x3f) == TF_LEVEL2 || (esr_el1->iss & 0x3f) == TF_LEVEL3)
    {
        // current address
        uart_printf("[Translation fault]: 0x%x\n", far_el1);

        size_t addr_offset = (far_el1 - the_area_ptr->virt_addr);
        // allign
        addr_offset = (addr_offset % 0x1000) == 0 ? addr_offset : addr_offset - (addr_offset % 0x1000);
        map_one_page_rwx(PHYS_TO_VIRT(curr_thread->context.ttbr0_el1), the_area_ptr->virt_addr + addr_offset, the_area_ptr->phys_addr + addr_offset, the_area_ptr->rwx);
    }
    // iss is other fault -> seg_fault
    else // For other Fault (permisson ...etc)
        seg_fault();
}

void map_one_page_rwx(size_t *pgd_p, size_t va, size_t pa, size_t rwxflag)
{
    size_t flag = 0;
    //set rwx and then map
    // execute
    if (!(rwxflag & 0b100))
        flag |= PD_UNX;

    // write
    if (!(rwxflag & 0b10))
        flag |= PD_RDONLY;

    // read
    if (rwxflag & 0b1)
        flag |= PD_UK_ACCESS;

    map_one_page(pgd_p, va, pa, flag);
}

void seg_fault()
{
    uart_printf("[Segmentation fault]: Kill Process\r\n");
    thread_exit();
}