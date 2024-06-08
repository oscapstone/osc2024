#include "bcm2837/rpi_mmu.h"
#include "mmu.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"
#include "stdint.h"
#include "sched.h"
#include "debug.h"
#include "syscall.h"

extern thread_t *curr_thread;
extern const char *IFSC_table[];

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

int set_thread_default_mmu(thread_t *t)
{
    mmu_add_vma(t, "Code", USER_CODE_BASE, t->datasize, (PROT_READ | PROT_EXEC), (VM_EXEC | VM_READ), t->code);
    mmu_add_vma(t, "User stack", USER_STACK_BASE - USTACK_SIZE + SP_OFFSET_FROM_TOP, USTACK_SIZE, (PROT_READ | PROT_WRITE), (VM_READ | VM_WRITE | VM_GROWSDOWN), NULL);
    mmu_add_vma(t, "Peripheral", PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, (PROT_READ | PROT_WRITE), (VM_READ | VM_WRITE | VM_PFNMAP), PERIPHERAL_START);
    mmu_add_vma(t,
                "Signal wrapper",                  // name
                USER_SIGNAL_WRAPPER_VA,            // virtual address
                0x2000,                            // size
                (PROT_READ | PROT_EXEC),           // page protection
                (VM_EXEC | VM_READ),               // vma flags
                (uint64_t)signal_handler_wrapper); // file
                                                   // ALIGN_DOWN((uint64_t)signal_handler_wrapper, PAGE_FRAME_SIZE)); // file
    mmu_add_vma(t,
                "Run user task wrapper",          // name
                USER_RUN_USER_TASK_WRAPPER_VA,    // virtual address
                0x2000,                           // size
                (PROT_READ | PROT_EXEC),          // page protection
                (VM_EXEC | VM_READ),              // vma flags
                (uint64_t)run_user_task_wrapper); // file
                                                  // ALIGN_DOWN((uint64_t)run_user_task_wrapper, PAGE_FRAME_SIZE)); // file
    return 0;
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
 * @param vm_file Pointer to the file associated with this VMA, if any.
 */
void mmu_add_vma(thread_t *t, char *name, size_t va, size_t size, uint64_t vm_page_prot, uint64_t vm_flags, char *vm_file)
{
    if (is_addr_not_align_to_page_size(va))
    {
        ERROR("mmu_add_vma: Address is not aligned to 4KB.\r\n");
        ERROR("VA: 0x%x,Size: 0x%x, vm_page_prot: 0x%x, vm_flags: 0x%x\r\n", va, size, vm_page_prot, vm_flags);
        while (1)
            ;
        return;
    }
    vm_area_struct_t *new_area = kmalloc(sizeof(vm_area_struct_t));
    size = ALIGN_UP(size, PAGE_FRAME_SIZE);
    new_area->name = name;
    new_area->start = va;
    new_area->end = va + size;
    new_area->vm_page_prot = vm_page_prot;
    new_area->vm_flags = vm_flags;
    new_area->vm_file = vm_file;
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
        mmu_free_vma((vm_area_struct_t *)curr);
    }
}

void mmu_free_vma(vm_area_struct_t *vma)
{
    list_del_entry((list_head_t *)vma);
    kfree(vma);
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
                DEBUG("Clean table: 0x%x, level: %d\r\n", (void *)PHYS_TO_KERNEL_VIRT((char *)next_table), level);
                if (level < LEVEL_PTE)
                {
                    DEBUG("kfree: 0x%x\r\n", (void *)PHYS_TO_KERNEL_VIRT((char *)next_table));
                    kfree(PHYS_TO_KERNEL_VIRT((char *)next_table));
                }
                else{
                    DEBUG("put_page: 0x%x\r\n", (void *)PHYS_TO_KERNEL_VIRT((char *)next_table));
                    put_page((uint64_t)PHYS_TO_KERNEL_VIRT((char *)next_table));
                }
            }
        }
    }
}

inline vm_area_struct_t *find_vma(thread_t *t, size_t va)
{
    list_head_t *pos;
    vm_area_struct_t *vma;
    DEBUG("---------------------------------------------- find vma: 0x%x ----------------------------------------------\r\n", va);
    list_for_each(pos, (list_head_t *)(t->vma_list))
    {
        vma = (vm_area_struct_t *)pos;
        DEBUG("vma: 0x%x, va: 0x%x - 0x%x\r\n", vma, vma->start, vma->end);
        if (vma->start <= va && vma->end > va)
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
    kernel_lock_interrupt();
    uint64_t far_el1;
    __asm__ __volatile__("mrs %0, FAR_EL1\n\t" : "=r"(far_el1));

    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *the_area_ptr = find_vma(curr_thread, far_el1);
    // area is not part of process's address space
    if (!the_area_ptr)
    {
        ERROR("[Segmentation fault]: 0x%x out of vma. Kill Process!\r\n", far_el1);
        ERROR_BLOCK({
            dump_vma(curr_thread);
        });
        kernel_unlock_interrupt();
        thread_exit();
        return;
    }
    DEBUG("the_area_ptr: 0x%x\r\n", the_area_ptr);

    size_t addr_offset = (far_el1 - the_area_ptr->start);
    addr_offset = ALIGN_DOWN(addr_offset, PAGE_FRAME_SIZE);
    size_t va_to_map = the_area_ptr->start + addr_offset;
    DEBUG("va_to_map: 0x%x, prot: 0x%x\r\n", va_to_map, the_area_ptr->vm_page_prot);

    uint8_t flag = 0;
    if (!(the_area_ptr->vm_page_prot & PROT_EXEC))
    {
        DEBUG("add non-executable flag\r\n");
        flag |= PD_UNX; // executable
        DEBUG("flag: 0x%x\r\n", flag);
    }
    if (the_area_ptr->vm_page_prot == PROT_READ)
    {
        DEBUG("add read-only flag\r\n");
        flag |= PD_RDONLY; // writable
    }
    if (the_area_ptr->vm_page_prot & PROT_READ)
    {
        DEBUG("add accessible flag\r\n");
        flag |= PD_UK_ACCESS; // readable / accessible
    }

    // For translation fault, only map one page frame for the fault address
    if ((esr_el1->iss & 0x3F) == TF_LEVEL0 ||
        (esr_el1->iss & 0x3F) == TF_LEVEL1 ||
        (esr_el1->iss & 0x3F) == TF_LEVEL2 ||
        (esr_el1->iss & 0x3F) == TF_LEVEL3)
    {
        INFO("[Translation fault]: 0x%x\r\n", far_el1); // far_el1: Fault address register.
                                                        // Holds the faulting Virtual Address for all synchronous Instruction or Data Abort, PC alignment fault and Watchpoint exceptions that are taken to EL1.

        DEBUG_BLOCK({
            dump_vma(curr_thread);
        });

        uint64_t ttbr0_el1_value;
        asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0_el1_value));
        DEBUG("pid: %d, curr_thread->context.pgd: 0x%x, ttbr0_el1: 0x%x\r\n", curr_thread->pid, curr_thread->context.pgd, ttbr0_el1_value);
        void *new_page;
        if (the_area_ptr->vm_flags & VM_PFNMAP)
        {
            DEBUG("PFNMAP: 0x%x\r\n", the_area_ptr->vm_file);
            new_page = (void *)(the_area_ptr->vm_file + addr_offset);
        }
        else
        {
            DEBUG("kmalloc: 0x%x\r\n", PAGE_FRAME_SIZE);
            new_page = kmalloc(PAGE_FRAME_SIZE);
            get_page(new_page);
            if (the_area_ptr->vm_file != NULL)
            {
                DEBUG("memcpy: 0x%x -> 0x%x, size: 0x%x\r\n", the_area_ptr->vm_file + addr_offset, new_page, PAGE_FRAME_SIZE);
                memcpy(new_page, the_area_ptr->vm_file + addr_offset, PAGE_FRAME_SIZE);
            }
            else
            {
                DEBUG("memset: 0x%x, size: 0x%x\r\n", new_page, PAGE_FRAME_SIZE);
                memset(new_page, 0, PAGE_FRAME_SIZE);
            }
        }
        map_one_page(PHYS_TO_KERNEL_VIRT(curr_thread->context.pgd), va_to_map, KERNEL_VIRT_TO_PHYS(new_page), flag);
        // map_one_page(PHYS_TO_KERNEL_VIRT(curr_thread->context.pgd), va_to_map, pa_to_map, flag);
        // dump_pagetable(va_to_map, pa_to_map);
        DEBUG("map one page done\r\n");
        kernel_unlock_interrupt();
        // schedule();
        // switch_mm_irqs_off(curr_thread->context.pgd); // flush tlb and pipeline because page table is change
        return;
    }
    else if (
        (esr_el1->iss & 0x3f) == PERMISSON_FAULT_LEVEL0 ||
        (esr_el1->iss & 0x3f) == PERMISSON_FAULT_LEVEL1 ||
        (esr_el1->iss & 0x3f) == PERMISSON_FAULT_LEVEL2 ||
        (esr_el1->iss & 0x3f) == PERMISSON_FAULT_LEVEL3)
    {
        INFO("[Permission fault]: 0x%x\r\n", far_el1); // far_el1: Fault address register.
                                                       // if (the_area_ptr->vm_page_prot & PROT_WRITE)
                                                       // {
                                                       //     size_t size = the_area_ptr->end - the_area_ptr->start;
                                                       //     DEBUG("size: 0x%x, the_area_ptr->phys_addr_area.start: 0x%x\r\n", size, the_area_ptr->phys_addr_area.start);
                                                       //     void *virt_new_pa = kmalloc(size);
                                                       //     DEBUG("memcpy: 0x%x -> 0x%x, size: 0x%x\r\n", PHYS_TO_KERNEL_VIRT(the_area_ptr->phys_addr_area.start), virt_new_pa, size);
                                                       //     memcpy(virt_new_pa, (void *)PHYS_TO_KERNEL_VIRT(the_area_ptr->phys_addr_area.start), size);
                                                       //     mmu_add_vma(curr_thread, the_area_ptr->name, the_area_ptr->start, size, KERNEL_VIRT_TO_PHYS(virt_new_pa), the_area_ptr->vm_page_prot, the_area_ptr->vm_flags, the_area_ptr->need_to_free);
                                                       //     for (int i = 0; i < size ; i += PAGE_FRAME_SIZE)
                                                       //     {
                                                       //         DEBUG("map_one_page: pgd: 0x%x 0x%x -> 0x%x, flag: 0x%x\r\n", PHYS_TO_KERNEL_VIRT(curr_thread->context.pgd), the_area_ptr->start + i, KERNEL_VIRT_TO_PHYS(virt_new_pa + i), flag);
                                                       //         map_one_page(PHYS_TO_KERNEL_VIRT(curr_thread->context.pgd), the_area_ptr->start + i, KERNEL_VIRT_TO_PHYS(virt_new_pa + i), flag);
                                                       //     }
                                                       //     mmu_free_vma(the_area_ptr);
                                                       //     dump_vma(curr_thread);
                                                       //     kernel_unlock_interrupt();
                                                       //     return;
                                                       // }
                                                       // else
                                                       // {
        // ERROR("Don't have write permission. Kill Process!\r\n");
        kernel_unlock_interrupt();
        thread_exit();
        return;
        // }
    }
    else
    {
        // For other Fault (permisson ...etc)
        char *IFSC_error_message = IFSC_table[(esr_el1->iss & 0x3f)];
        ERROR("[Segmentation fault]: %s. Kill Process!\r\n", IFSC_error_message);
        kernel_unlock_interrupt();
        thread_exit();
        return;
    }
}

void mmu_set_page_table_read_only(size_t *pgd, PAGE_TABLE_LEVEL level)
{
    // size_t *table_virt = (size_t *)PHYS_TO_KERNEL_VIRT((char *)pgd);
    // for (int i = 0; i < 512; i++)
    // {
    //     if (table_virt[i] != 0)
    //     {
    //         size_t *next_table = (size_t *)(table_virt[i] & ENTRY_ADDR_MASK);
    //         if (table_virt[i] & PD_TABLE)
    //         {
    //             if (level <= LEVEL_PMD) // not the last level
    //             {
    //                 DEBUG("next_table: 0x%x, level: %d\r\n", next_table, level + 1);
    //                 mmu_set_page_table_read_only(next_table, level + 1);
    //             }
    //             else
    //             {
    //                 DEBUG("set read-only: 0x%x, level: %d\r\n", table_virt[i], level);
    //                 table_virt[i] |= PD_RDONLY;
    //             }
    //         }
    //     }
    // }
}

void mmu_reset_page_tables_read_only(size_t *parent_table, size_t *child_table, int level)
{
    if (level > 3)
        return;

    size_t *parent_table_virt = (size_t *)PHYS_TO_KERNEL_VIRT((char *)parent_table);
    size_t *child_table_virt = (size_t *)PHYS_TO_KERNEL_VIRT((char *)child_table);
    // int counter_ = 0;
    for (int i = 0; i < 512; i++)
    {
        if (parent_table_virt[i] == 0)
            continue;

        if (!(parent_table_virt[i] & PD_TABLE))
        {
            DEBUG("mmu_reset_page_tables_read_only error\r\n");
            return;
        }
        if (level == 3)
        {
            child_table_virt[i] = parent_table_virt[i];
            child_table_virt[i] |= PD_RDONLY;
            // if (counter_ < 3)
            parent_table_virt[i] |= PD_RDONLY;
            // counter_++;
        }
        else
        {
            if (!child_table_virt[i])
            {
                size_t *new_table = kmalloc(0x1000);
                memset(new_table, 0, 0x1000);
                child_table_virt[i] = KERNEL_VIRT_TO_PHYS((size_t)new_table);
                child_table_virt[i] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2);
            }
            size_t *parent_next_table = (size_t *)(parent_table_virt[i] & ENTRY_ADDR_MASK);
            size_t *child_next_table = (size_t *)(child_table_virt[i] & ENTRY_ADDR_MASK);
            mmu_reset_page_tables_read_only(parent_next_table, child_next_table, level + 1);
        }
    }
}

void mmu_copy_page_table(size_t *dest, size_t *src, PAGE_TABLE_LEVEL level)
{
    size_t *dest_table_virt = (size_t *)PHYS_TO_KERNEL_VIRT((char *)dest);
    size_t *src_table_virt = (size_t *)PHYS_TO_KERNEL_VIRT((char *)src);
    for (int i = 0; i < 512; i++)
    {
        if (src_table_virt[i] != 0)
        {
            size_t *next_table = (size_t *)(src_table_virt[i] & ENTRY_ADDR_MASK);
            if (src_table_virt[i] & PD_TABLE)
            {
                if (level <= LEVEL_PMD) // not the last level
                {
                    dest_table_virt[i] = KERNEL_VIRT_TO_PHYS((size_t)kmalloc(PAGE_FRAME_SIZE));
                    mmu_copy_page_table((size_t *)(dest_table_virt[i] & ENTRY_ADDR_MASK), next_table, level + 1);
                }
                else
                {
                    DEBUG("copy page: 0x%x -> 0x%x, level: %d\r\n", &(src_table_virt[i]), &(dest_table_virt[i]), level);
                    src_table_virt[i] |= PD_RDONLY;
                    dest_table_virt[i] = src_table_virt[i];
                }
            }
        }
    }
}

void dump_vma(thread_t *t)
{
    list_head_t *pos;
    vm_area_struct_t *vma;
    uart_puts("=================================================================== VMA ===================================================================\r\n");
    list_for_each(pos, (list_head_t *)(t->vma_list))
    {
        vma = (vm_area_struct_t *)pos;
        int len = strlen(vma->name);
        uart_puts("%s ", vma->name);
        for (int i = 0; i < 23 - len; i++)
            uart_puts(" ");
        uart_puts("VMA: 0x%x, VA: 0x%x - 0x%x, Page prot: (",
                  vma,
                  vma->start, vma->end);
        if (vma->vm_page_prot & PROT_READ)
            uart_puts("R");
        if (vma->vm_page_prot & PROT_WRITE)
            uart_puts("W");
        if (vma->vm_page_prot & PROT_EXEC)
            uart_puts("X");
        uart_puts("), Flags: 0x%x\r\n", vma->vm_flags);
    }
    uart_puts("=================================================================== END ===================================================================\r\n");
}

#define DUMP_NAME(number, name) \
    case number:                \
        uart_puts(name);        \
        uart_puts("\n");        \
        break;

typedef enum
{
    UNKNOW_AREA = -1,
    USER_DATA,
    USER_STACK,
    PERIPHERAL,
    USER_SIGNAL_WRAPPER
} vma_name_type;

typedef enum
{
    PGD,
    PUD,
    PMD,
    PTE,
} pagetable_type;

void dump_pagetable(unsigned long user_va, unsigned long pa)
{
    uart_puts("      +---------------------------+\n");
    uart_puts("      | DUMP PAGE TABLE & ADDRESS |\n");
    uart_puts("      +---------------------------+\n");
    unsigned long *pagetable_pa = curr_thread->context.pgd;
    unsigned long *pagetable_kernel_va = PHYS_TO_KERNEL_VIRT(pagetable_pa);

    uart_puts("===============================================\n");
    uart_puts("User Physical Address : 0x%x\n", pa);
    uart_puts("User Vitural  Address : 0x%x\n", user_va);
    unsigned long offset = user_va & 0xFFF;
    uart_puts("Vitural Address offset: 0x%x\n", offset);
    for (int level = 0; level < 4; level++)
    {
        uart_puts("-----------------------------------------------\n");
        uart_puts("PAGE TABLE : ");
        switch (level)
        {
            DUMP_NAME(PGD, "PGD")
            DUMP_NAME(PUD, "PUD")
            DUMP_NAME(PMD, "PMD")
            DUMP_NAME(PTE, "PTE")
        default:
            uart_puts("unnamed: %d\n", level);
            break;
        }
        uart_puts("Pagetable Vitural  Address: 0x%x\n", pagetable_kernel_va);
        uart_puts("Pagetable Physical Address: 0x%x\n", pagetable_pa);

        unsigned int idx = (user_va >> (39 - level * 9)) & 0x1FF;
        uart_puts("Index of Pagetable: 0x%x\n", idx);
        uart_puts("Entry %x of Pagetable: 0x%x\n", idx, pagetable_kernel_va[idx]);
        if (level == PTE)
        {
            pagetable_pa = pagetable_kernel_va[idx] & ENTRY_ADDR_MASK;
            uart_puts("The Page Base Physical Address: 0x%x\n", pagetable_pa);
            uart_puts("The Physical Address: 0x%x\n", (unsigned long)pagetable_pa | offset);
        }
        else
        {
            pagetable_pa = pagetable_kernel_va[idx] & ENTRY_ADDR_MASK;
            uart_puts("next Pagetable Physical Address: 0x%x\n", pagetable_pa);
            pagetable_kernel_va = PHYS_TO_KERNEL_VIRT(pagetable_pa);
            uart_puts("next Pagetable Vitural  Address: 0x%x\n", pagetable_kernel_va);
        }
        uart_puts("-----------------------------------------------\n");
    }
}