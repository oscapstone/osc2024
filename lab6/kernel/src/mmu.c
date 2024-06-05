#include "bcm2837/rpi_mmu.h"
#include "mmu.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"
#include "stdint.h"
#include "sched.h"
#include "debug.h"

// e.g. size=0x13200, alignment=0x1000 -> 0x14000
#define ALIGN_UP(size, alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))
// e.g. size=0x13200, alignment=0x1000 -> 0x13000
#define ALIGN_DOWN(size, alignment) ((size) & ~((alignment) - 1))

extern thread_t *curr_thread;

void *set_2M_kernel_mmu(void *x0)
{
    // Turn
    //   Two-level Translation (1GB) - in boot.S
    // to
    //   Three-level Translation (2MB) - set PUD point to new table
    uint64_t *pud_table = (uint64_t *)MMU_PUD_ADDR;

    uint64_t *pte_table1 = (uint64_t *)MMU_PTE_ADDR;
    uint64_t *pte_table2 = (uint64_t *)(MMU_PTE_ADDR + 0x1000L);
    for (int i = 0; i < 512; i++)
    {
        uint64_t addr = 0x200000L * i;
        if (addr >= PERIPHERAL_END)
        {
            pte_table1[i] = (0x00000000 + addr) + BOOT_PTE_ATTR_nGnRnE;
            continue;
        }
        pte_table1[i] = (0x00000000 + addr) | BOOT_PTE_ATTR_NOCACHE; //   0 * 2MB // No definition for 3-level attribute, use nocache.
        pte_table2[i] = (0x40000000 + addr) | BOOT_PTE_ATTR_NOCACHE; // 512 * 2MB
    }

    // set PUD
    pud_table[0] = (uint64_t)pte_table1 | BOOT_PUD_ATTR;
    pud_table[1] = (uint64_t)pte_table2 | BOOT_PUD_ATTR;

    return x0;
}

static inline is_addr_not_align_to_page_size(size_t addr)
{
    return ALIGN_UP(addr, PAGE_FRAME_SIZE) != addr;
}

/**
 * @brief Map virtual address to physical address with flags in page table
 *
 * @param virt_pgd_p: kernel virtual address of user space pgd
 * @param va: user virtual address (aligned to 4KB)
 * @param pa: physical address (aligned to 4KB)
 * @param flag: flags
 */
void map_one_page(size_t *virt_pgd_p, size_t va, size_t pa, size_t flag)
{
    if (is_addr_not_align_to_page_size(va) || is_addr_not_align_to_page_size(pa))
    {
        ERROR("map_one_page: Address is not aligned to 4KB.\r\n");
        ERROR("PGD: 0x%x, VA: 0x%x, PA: 0x%x, Flag: 0x%x\r\n", virt_pgd_p, va, pa, flag);
        return;
    }
    size_t *table_p = virt_pgd_p;
    for (int level = 0; level < 4; level++)
    {
        unsigned int idx = (va >> (39 - level * 9)) & 0x1ff; // p.14, 9-bit only

        if (level == 3)
        {
            table_p[idx] = pa;
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KNX | flag; // el0 only
            DEBUG("Map VA: 0x%x to PA: 0x%x with flag: 0x%x\r\n", va, pa, flag);
            return;
        }

        if (!table_p[idx])
        {
            size_t *newtable_p = kmalloc(PAGE_FRAME_SIZE); // create a table
            memset(newtable_p, 0, PAGE_FRAME_SIZE);
            table_p[idx] = KERNEL_VIRT_TO_PHYS((size_t)newtable_p); // point to that table
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2);
        }

        table_p = (size_t *)PHYS_TO_KERNEL_VIRT((size_t)(table_p[idx] & ENTRY_ADDR_MASK)); // PAGE_SIZE
    }
}

/**
 * @brief Add a virtual memory area to a thread
 *
 * @param t Pointer to the thread structure to which the VMA will be added.
 * @param name Name of the virtual memory area.
 * @param va Virtual address where the VMA starts.
 * @param size Size of the VMA in bytes. This will be aligned up to the nearest page size.
 * @param pa Physical address corresponding to the start of the VMA.
 * @param vm_page_prot Page protection attributes for the VMA.
 * @param vm_flags Flags indicating the properties and permissions of the VMA.
 * @param need_to_free Flag indicating whether the physical page needs to be freed.
 */
void mmu_add_vma(thread_t *t, char *name, size_t va, size_t size, size_t pa, uint64_t vm_page_prot, uint64_t vm_flags, uint8_t need_to_free)
{
    if (is_addr_not_align_to_page_size(va) || is_addr_not_align_to_page_size(pa))
    {
        ERROR("mmu_add_vma: Address is not aligned to 4KB.\r\n");
        ERROR("VA: 0x%x, PA: 0x%x, Size: 0x%x, vm_page_prot: 0x%x, vm_flags: 0x%x, need_to_free: 0x%x\r\n", va, pa, size, vm_page_prot, vm_flags, need_to_free);
        return;
    }
    vm_area_struct_t *new_area = kmalloc(sizeof(vm_area_struct_t));
    size = ALIGN_UP(size, PAGE_FRAME_SIZE);
    new_area->name = name;
    new_area->virt_addr_area.start = va;
    new_area->virt_addr_area.end = va + size;
    new_area->phys_addr_area.start = pa;
    new_area->phys_addr_area.end = pa + size;
    new_area->vm_page_prot = vm_page_prot;
    new_area->vm_flags = vm_flags;
    new_area->need_to_free = need_to_free;
    list_add_tail((list_head_t *)new_area, (list_head_t *)(t->vma_list));
}

/**
 * @brief Delete all VMAs (Virtual Memory Areas) in the thread and free the physical addresses mapped by those VMAs
 *
 * @param t: thread
 */
void mmu_free_all_vma(thread_t *t)
{
    list_head_t *curr = (list_head_t *)t->vma_list;
    list_head_t *n;
    list_for_each_safe(curr, n, (list_head_t *)t->vma_list)
    {
        vm_area_struct_t *vma = (vm_area_struct_t *)curr;
        if (vma->need_to_free)
            kfree((void *)PHYS_TO_KERNEL_VIRT(vma->phys_addr_area.start));
        list_del_entry(curr);
        kfree(curr);
    }
}

/**
 * @brief Clear all page tables and free PUD, PMD, PTE tables while preserving PGD.
 *
 * @param page_table: the page table to be cleaned, usually the PGD
 * @param level: the level of the page table, usually LEVEL_PGD
 */
void mmu_clean_page_tables(size_t *page_table, PAGE_TABLE_LEVEL level)
{
    size_t *table_virt = (size_t *)PHYS_TO_KERNEL_VIRT((char *)page_table);
    for (int i = 0; i < 512; i++)
    {
        if (table_virt[i] != 0)
        {
            size_t *next_table = (size_t *)(table_virt[i] & ENTRY_ADDR_MASK);
            if (table_virt[i] & PD_TABLE)
            {
                if (level < LEVEL_PTE) // not the last level
                    mmu_clean_page_tables(next_table, level + 1);
                table_virt[i] = 0L;
                kfree(PHYS_TO_KERNEL_VIRT((char *)next_table));
            }
        }
    }
}

static inline vm_area_struct_t *find_vma(thread_t *t, size_t va)
{
    list_head_t *pos;
    vm_area_struct_t *vma;
    DEBUG("---------------------------------------------- find vma: 0x%x ----------------------------------------------\r\n", va);
    list_for_each(pos, (list_head_t *)(t->vma_list))
    {
        vma = (vm_area_struct_t *)pos;
        DEBUG("vma: 0x%x, va: 0x%x - 0x%x\r\n", vma, vma->virt_addr_area.start, vma->virt_addr_area.end);
        if (vma->virt_addr_area.start <= va && vma->virt_addr_area.end > va)
        {
            DEBUG("----------------------------------------------------- end -----------------------------------------------------\r\n", va);
            return vma;
        }
    }
    DEBUG("----------------------------------------------------- end -----------------------------------------------------\r\n", va);
    return 0;
}

void mmu_memfail_abort_handle(esr_el1_t *esr_el1)
{
    uint64_t far_el1;
    __asm__ __volatile__("mrs %0, FAR_EL1\n\t" : "=r"(far_el1));

    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *the_area_ptr = find_vma(curr_thread, far_el1);
    // area is not part of process's address space
    if (!the_area_ptr)
    {
        ERROR("[Segmentation fault]: 0x%x out of vma. Kill Process!\r\n", far_el1);
        thread_exit();
        return;
    }
    DEBUG("the_area_ptr: 0x%x\r\n", the_area_ptr);

    // For translation fault, only map one page frame for the fault address
    if ((esr_el1->iss & 0x3F) == TF_LEVEL0 ||
        (esr_el1->iss & 0x3F) == TF_LEVEL1 ||
        (esr_el1->iss & 0x3F) == TF_LEVEL2 ||
        (esr_el1->iss & 0x3F) == TF_LEVEL3)
    {
        INFO("[Translation fault]: 0x%x\r\n", far_el1); // far_el1: Fault address register.
                                                        // Holds the faulting Virtual Address for all synchronous Instruction or Data Abort, PC alignment fault and Watchpoint exceptions that are taken to EL1.

        size_t addr_offset = (far_el1 - the_area_ptr->virt_addr_area.start);
        addr_offset = ALIGN_DOWN(addr_offset, PAGE_FRAME_SIZE);
        size_t va_to_map = the_area_ptr->virt_addr_area.start + addr_offset;
        size_t pa_to_map = the_area_ptr->phys_addr_area.start + addr_offset;
        DEBUG("va_to_map: 0x%x, pa_to_map: 0x%x, rwx: 0x%x\r\n", va_to_map, pa_to_map, the_area_ptr->rwx);

        dump_vma(curr_thread);

        uint8_t flag = 0;
        if (!(the_area_ptr->vm_page_prot & PROT_EXEC))
            flag |= PD_UNX; // executable
        if (!(the_area_ptr->vm_page_prot & PROT_WRITE))
            flag |= PD_RDONLY; // writable
        if (the_area_ptr->vm_page_prot & PROT_READ)
            flag |= PD_UK_ACCESS; // readable / accessible
        DEBUG("PGD: 0x%x\r\n", curr_thread->context.pgd);
        map_one_page(PHYS_TO_KERNEL_VIRT(curr_thread->context.pgd), va_to_map, pa_to_map, flag);
    }
    else
    {
        // For other Fault (permisson ...etc)
        ERROR("[Segmentation fault]: Other Fault. Kill Process!\r\n");
        thread_exit();
    }
}

void dump_vma(thread_t *t)
{
    list_head_t *pos;
    vm_area_struct_t *vma;
    INFO("=================================================================== VMA ===================================================================\r\n");
    list_for_each(pos, (list_head_t *)(t->vma_list))
    {
        vma = (vm_area_struct_t *)pos;
        INFO_BLOCK({
            int len = strlen(vma->name);
            INFO("%s ", vma->name);
            for (int i = 0; i < 23 - len; i++)
                uart_puts(" ");
            uart_puts("VMA: 0x%x, VA: 0x%x - 0x%x, PA: 0x%x - 0x%x, RWX: 0x%x, Need to free: 0x%x\r\n",
                      vma,
                      vma->virt_addr_area.start, vma->virt_addr_area.end,
                      vma->phys_addr_area.start, vma->phys_addr_area.end,
                      vma->rwx, vma->need_to_free);
        });
    }
    INFO("=================================================================== END ===================================================================\r\n");
}