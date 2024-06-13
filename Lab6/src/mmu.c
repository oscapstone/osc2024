#include "bcm2837/rpi_mmu.h"
#include "mmu.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"

void *set_2M_kernel_mmu(void *x0)
{
    unsigned long *pud_table = (unsigned long *)MMU_PUD_ADDR;

    unsigned long *pmd_table1 = (unsigned long *)MMU_PMD_ADDR;
    unsigned long *pmd_table2 = (unsigned long *)(MMU_PMD_ADDR + 0x1000L);
    for (int i = 0; i < 512; i++)
    {
        // one entry point to 2M region
        unsigned long addr = 0x200000L * i;
        if (addr >= PERIPHERAL_END)
        {
            pmd_table1[i] = (0x00000000 + addr) + BOOT_PMD_ATTR_nGnRnE;
            continue;
        }
        pmd_table1[i] = (0x00000000 + addr) | BOOT_PMD_ATTR_NOCACHE; //   0 * 2MB
        pmd_table2[i] = (0x40000000 + addr) | BOOT_PMD_ATTR_NOCACHE; // 512 * 2MB
    }

    // set PUD
    pud_table[0] = (unsigned long)pmd_table1 | BOOT_PUD_ATTR;
    pud_table[1] = (unsigned long)pmd_table2 | BOOT_PUD_ATTR;

    return x0;
}

// pa,va aligned to 4K
void map_one_page(size_t *virt_pgd_p, size_t va, size_t pa, size_t flag)
{
    size_t *table_p = virt_pgd_p;
    for (int level = 0; level < 4; level++)
    {
        unsigned int idx = (va >> (39 - level * 9)) & 0x1ff; // 0x1ff => 9 1s 9 bits as a unit

        // point to a page (level 3)
        if (level == 3)
        {
            /*
                Entry of PTE which points to a page
                +-----+------------------------------+---------+--+
                |     |  page's physical address     |attribute|11|
                +-----+------------------------------+---------+--+
                     47                             12         2  0
            */
            table_p[idx] = pa;
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KNX | flag;  // el0 only
            return;
        }

        // the entry is null
        if (!table_p[idx])
        {
            /*
                Entry of PGD, PUD, PMD which point to a page table
                +-----+------------------------------+---------+--+
                |     | next level table's phys addr | ignored |11|
                +-----+------------------------------+---------+--+
                     47                             12         2  0
            */
            size_t *newtable_p = kmalloc(0x1000);            // new table
            memset(newtable_p, 0, 0x1000);                   // newtable_p = 0 * 0x1000 (0000..000)
            table_p[idx] = VIRT_TO_PHYS((size_t)newtable_p); // point to a new page table
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2);
        }

        table_p = (size_t *)PHYS_TO_VIRT((size_t)(table_p[idx] & ENTRY_ADDR_MASK)); // mask control bits keep pysical address to make sure correct address
    }
}

// add virtual memory area to the thread's list
void mmu_add_vma(struct thread *t, size_t va, size_t size, size_t pa, size_t rwx, int is_alloced)
{
    size = size % 0x1000 ? size + (0x1000 - size % 0x1000) : size;  //  align size to 4K
    vm_area_struct_t* new_area = kmalloc(sizeof(vm_area_struct_t));
    new_area->rwx = rwx;
    new_area->area_size = size;
    new_area->virt_addr = va;
    new_area->phys_addr = pa;
    new_area->is_alloced = is_alloced;
    list_add_tail((list_head_t *)new_area, &t->vma_list);
}

void mmu_del_vma(struct thread *t)
{
    list_head_t *pos = t->vma_list.next;
    vm_area_struct_t *vma;
    // free not being used vma
    while(pos != &t->vma_list){
        vma = (vm_area_struct_t *)pos;
        if (vma->is_alloced)
            kfree((void*)PHYS_TO_VIRT(vma->phys_addr));
        list_head_t* next_pos = pos->next;
        kfree(pos);
        pos = next_pos;
    }
}

// give
void mmu_map_pages(size_t *virt_pgd_p, size_t va, size_t size, size_t pa, size_t flag)
{
    pa = pa - (pa % 0x1000); // align
    for (size_t s = 0; s < size; s += 0x1000)
    {
        map_one_page(virt_pgd_p, va + s, pa + s, flag);
    }
}

void mmu_free_page_tables(size_t *page_table, int level)
{
    size_t *table_virt = (size_t *)PHYS_TO_VIRT((char *)page_table);
    for (int i = 0; i < 512; i++)
    {
        if (table_virt[i] != 0)
        {
            //  next level table
            size_t *next_table = (size_t *)(table_virt[i] & ENTRY_ADDR_MASK); // get pa
            if (table_virt[i] & PD_TABLE)   //table or page
            {
                // level 3 won't point to table
                // free upper level table who points to this table
                if(level!=2)mmu_free_page_tables(next_table, level + 1);
                table_virt[i] = 0L;
                kfree(PHYS_TO_VIRT((char *)next_table));
            }
        }
    }
}


void mmu_memfail_abort_handle(esr_el1_t* esr_el1)
{
    //https://developer.arm.com/documentation/ddi0500/j/System-Control/AArch64-register-descriptions/Fault-Address-Register--EL1
    //Holds the faulting Virtual Address for all synchronous instruction or data aborts, or exceptions from a misaligned PC or a Watchpoint debug event, taken to EL1.
    unsigned long long far_el1;
    __asm__ __volatile__("mrs %0, FAR_EL1\n\t": "=r"(far_el1));
    uart_sendline("MMU Fail, FAR_EL1: 0x%x\r\n", far_el1);
    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *the_area_ptr = 0;
    // find the vma have fault 
    list_for_each(pos, &curr_thread->vma_list)
    {
        vma = (vm_area_struct_t *)pos;
        //uart_sendline("vma->virt_addr: 0x%x, rwx: %d\r\n", vma->virt_addr, vma->rwx);
        if (vma->virt_addr <= far_el1 && vma->virt_addr + vma->area_size >= far_el1)
        {
            the_area_ptr = vma;
            break;
        }
    }

    //If the fault address is not part of any region in the process’s address space,
    if (!the_area_ptr)
    {
        uart_sendline("[Segmentation fault - not in region]: Kill Process\r\n");
        thread_exit();
        return;
    }

    //If it’s part of one region, the kernel takes addr as a hint and decides the new region’s start address.
    // For translation fault
    if ((esr_el1->iss & 0x3f) == TF_LEVEL0 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL1 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL2 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL3)
    {
        uart_sendline("[Translation fault]: 0x%x\r\n",far_el1);

        size_t addr_offset = (far_el1 - the_area_ptr->virt_addr);
        addr_offset = (addr_offset % 0x1000) == 0 ? addr_offset : addr_offset - (addr_offset % 0x1000);

        size_t flag = 0;    
        if(!(the_area_ptr->rwx & (0b1 << 2))) flag |= PD_UNX;   // 4: unexecutable
        if(!(the_area_ptr->rwx & (0b1 << 1))) flag |= PD_RDONLY;    // 2: unwritable => read only
        if(  the_area_ptr->rwx & (0b1 << 0) ) flag |= PD_UK_ACCESS; // 1: readable / accessible
        map_one_page(PHYS_TO_VIRT(curr_thread->context.pgd), the_area_ptr->virt_addr + addr_offset, the_area_ptr->phys_addr + addr_offset, flag);
    }
    else
    {
        // For other Fault (permisson ...etc)
        uart_sendline("[Segmentation fault - other]: Kill Process\r\n");
        thread_exit();
    }

}