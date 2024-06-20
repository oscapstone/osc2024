#include "bcm2837/rpi_mmu.h"
#include "mmu.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"
#include "debug.h"

extern thread_t *curr_thread;

void *set_2M_kernel_mmu(void *x0)
{
    // Turn
    //   Two-level Translation (1GB) - in boot.S
    // to
    //   Three-level Translation (2MB) - set PUD point to new table
    unsigned long *pud_table = (unsigned long *)MMU_PUD_ADDR;

    unsigned long *pte_table1 = (unsigned long *)MMU_PTE_ADDR;
    unsigned long *pte_table2 = (unsigned long *)(MMU_PTE_ADDR + 0x1000L);
    for (int i = 0; i < 512; i++)
    {
        unsigned long addr = 0x200000L * i;
        if (addr >= PERIPHERAL_END)
        {
            pte_table1[i] = (0x00000000 + addr) + BOOT_PTE_ATTR_nGnRnE;
            continue;
        }
        pte_table1[i] = (0x00000000 + addr) | BOOT_PTE_ATTR_NOCACHE; //   0 * 2MB // No definition for 3-level attribute, use nocache.
        pte_table2[i] = (0x40000000 + addr) | BOOT_PTE_ATTR_NOCACHE; // 512 * 2MB
    }

    // set PUD
    pud_table[0] = (unsigned long)pte_table1 | BOOT_PUD_ATTR;
    pud_table[1] = (unsigned long)pte_table2 | BOOT_PUD_ATTR;

    return x0;
}

void map_one_page(size_t *virt_pgd_p, size_t user_va, size_t pa, size_t flag)
{
    size_t *table_p = virt_pgd_p;
    for (int level = 0; level < 4; level++)
    {
        unsigned int idx = (user_va >> (39 - level * 9)) & 0x1ff; // p.14, 9-bit only

        if (level == 3)
        {
            table_p[idx] = pa;
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KNX | flag; // el0 only
            return;
        }

        if (!table_p[idx])
        {
            size_t *newtable_p = kmalloc(0x1000); // create a table
            memset(newtable_p, 0, 0x1000);
            table_p[idx] = KERNEL_VIRT_TO_PHYS((size_t)newtable_p); // point to that table
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2);
        }

        table_p = (size_t *)PHYS_TO_KERNEL_VIRT((size_t)(table_p[idx] & ENTRY_ADDR_MASK)); // PAGE_SIZE
    }
}

void mmu_add_vma(struct thread *t, size_t va, size_t size, size_t pa, size_t rwx, int is_alloced, vma_name_type name)
{
    if (IS_NOT_ALIGN(pa, PAGESIZE) || IS_NOT_ALIGN(va, PAGESIZE))
    {
        // ERROR("CHECK_ALIGN : 0x%x\n",CHECK_ALIGN(pa,PAGESIZE));
        uart_sendlinek("\n\n");
        ERROR("Input User Vitural Address or Physical Address Should be Aliged to PAGESIZE\n");
        ERROR("Input User Vitural Address : 0x%x\n", va);
        ERROR("Input Physical Address : 0x%x\n", pa);
        return;
    }
    vm_area_struct_t *the_area_ptr = check_vma_overlap(t, va, (unsigned long)size);
    if (the_area_ptr != 0)
    {
        uart_sendlinek("\n\n");
        ERROR("check_vma_overlap : 0x%x\n", the_area_ptr);
        ERROR("Vitural Memory Area is Overlap !!\n");
        // dump_vma();
        return;
    }

    size = ALIGN_UP(size, PAGESIZE);
    vm_area_struct_t *new_area = kmalloc(sizeof(vm_area_struct_t));
    new_area->rwx = rwx;
    new_area->area_size = size;
    new_area->virt_addr = va;
    new_area->phys_addr = pa;
    new_area->is_alloced = is_alloced;
    new_area->name = name;
    list_add_tail((list_head_t *)new_area, &t->vma_list);
}

void mmu_del_vma(struct thread *t)
{
    list_head_t *curr = &t->vma_list;
    list_head_t *n;
    list_for_each_safe(curr, n, &t->vma_list)
    {
        vm_area_struct_t *vma = (vm_area_struct_t *)curr;
        if (vma->is_alloced)
        {
            kfree((void *)PHYS_TO_KERNEL_VIRT(vma->phys_addr));
        }
        list_del_entry(curr);
        kfree(curr);
    }
}

void mmu_free_page_tables(size_t *page_table, int level)
{
    size_t *table_virt = (size_t *)PHYS_TO_KERNEL_VIRT(page_table);
    for (int i = 0; i < 512; i++)
    {
        if (table_virt[i] != 0)
        {
            size_t *next_table = (size_t *)(table_virt[i] & ENTRY_ADDR_MASK);
            if ((table_virt[i] & PD_TABLE) == PD_TABLE)
            {
                if (level < PMD)
                    mmu_free_page_tables(next_table, level + 1);
                table_virt[i] = 0L;
                kfree((void *)PHYS_TO_KERNEL_VIRT(next_table));
            }
        }
    }
}

// void mmu_set_PTE_readonly(size_t *page_table, int level)
// {
//     size_t *table_virt = (size_t *)PHYS_TO_KERNEL_VIRT(page_table);
//     for (int i = 0; i < 512; i++)
//     {
//         if (table_virt[i] != 0)
//         {
//             size_t *next_table = (size_t)(table_virt[i] & ENTRY_ADDR_MASK);
//             if (table_virt[i] & PD_TABLE == PD_TABLE)
//             {
//                 if (level < PMD)
//                 {
//                     mmu_set_PTE_readonly(next_table, level + 1);
//                 }
//                 else
//                 {
//                     table_virt[i] &= PD_RDONLY;
//                 }
//             }
//         }
//     }
// }

// void mmu_pagetable_copy(size_t *dst_page_table, size_t *src_page_table, int level)
// {
//     size_t *dst_page_table_va = (size_t *)PHYS_TO_KERNEL_VIRT(dst_page_table);
//     size_t *src_page_table_va = (size_t *)PHYS_TO_KERNEL_VIRT(src_page_table);
//     for (int i = 0; i < 512; i++)
//     {
//         if (src_page_table_va[i] != 0)
//         {
//             size_t *next_src_table = (size_t)(src_page_table_va[i] & ENTRY_ADDR_MASK);
//             size_t *next_dst_table = kmalloc(PAGESIZE);
//             memset(next_dst_table, 0, 0x1000);
//             dst_page_table_va[i] = KERNEL_VIRT_TO_PHYS((size_t)next_dst_table); // point to that table
//             dst_page_table_va[i] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2);
//             // size_t *next_dst_table = (size_t)(dst_page_table_va[i] & ENTRY_ADDR_MASK);
//             if (src_page_table_va[i] & PD_TABLE == PD_TABLE)
//             {
//                 if (level < PTE)
//                 {
//                     mmu_pagetable_copy(next_dst_table,next_src_table,level);
//                 }
//                 else
//                 {
//                     //--------
//                 }
//             }
//         }
//     }
// }

void mmu_memfail_abort_handle(esr_el1_t *esr_el1)
{
    lock();
    unsigned long long far_el1;
    __asm__ __volatile__("mrs %0, FAR_EL1\n\t" : "=r"(far_el1));

    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *the_area_ptr = 0;
    list_for_each(pos, &curr_thread->vma_list)
    {
        vma = (vm_area_struct_t *)pos;
        if (vma->virt_addr <= far_el1 && vma->virt_addr + vma->area_size >= far_el1)
        {
            the_area_ptr = vma;
            break;
        }
    }
    // area is not part of process's address space
    if (!the_area_ptr)
    {
        uart_sendlinek("\n\n");
        ERROR("[Segmentation fault]: Kill Process\r\n");
        ERROR("Invilad Vitural Address Access: %x\n", far_el1);
        thread_exit();
        // dump_vma();
        unlock();
        while (1)
            schedule();

        return;
    }

    // For translation fault, only map one page frame for the fault address
    if ((esr_el1->iss & 0x3f) == TF_LEVEL0 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL1 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL2 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL3)
    {
        // uart_sendlinek("\n");
        // WARING("[Translation fault]: 0x%x\r\n", far_el1); // far_el1: Fault address register.
                                                          // Holds the faulting Virtual Address for all synchronous Instruction or Data Abort, PC alignment fault and Watchpoint exceptions that are taken to EL1.

        size_t addr_offset = (far_el1 - the_area_ptr->virt_addr);
        // addr_offset = (addr_offset % 0x1000) == 0 ? addr_offset : addr_offset - (addr_offset % 0x1000);
        addr_offset = ALIGN_DOWN(addr_offset, 0x1000);

        size_t flag = 0;
        if (!(the_area_ptr->rwx & (0b1 << 2)))
            flag |= PD_UNX; // 4: executable
        if (!(the_area_ptr->rwx & (0b1 << 1)))
            flag |= PD_RDONLY; // 2: writable
        if (the_area_ptr->rwx & (0b1 << 0))
            flag |= PD_UK_ACCESS; // 1: readable / accessible
        map_one_page((size_t *)PHYS_TO_KERNEL_VIRT(curr_thread->context.pgd), the_area_ptr->virt_addr + addr_offset, the_area_ptr->phys_addr + addr_offset, flag);
        // dump_pagetable(the_area_ptr->virt_addr + addr_offset,the_area_ptr->phys_addr + addr_offset);
    }
    else
    {
        // For other Fault (permisson ...etc)
        // uart_sendlinek("[Other Fault]: Kill Process\r\n");
        // uart_sendlinek("esr_el1: 0x%x\r\n", esr_el1);
        uart_sendlinek("\n\n");
        if ((unsigned long)esr_el1 & (1 << 10))
        {
            ERROR("[Permission Fault] due to a write of an Allocation Tag to Canonically Tagged memory.\r\n");
        }
        if ((unsigned long)esr_el1 & (1 << 9))
        {
            ERROR("[Permission Fault] due to the NoTagAccess memory attribute..\r\n");
        }
        // check_permission(,the_area_ptr->rwx);
        thread_exit();
        // dump_vma();
        unlock();
        while (1)
            schedule();
    }
    unlock();
}

vm_area_struct_t *check_vma_overlap(thread_t *t, unsigned long user_va, unsigned long size)
{
    list_head_t *pos;
    vm_area_struct_t *vma;
    list_for_each(pos, &t->vma_list)
    {
        vma = (vm_area_struct_t *)pos;
        // Detect existing vma overlapped
        if (!(vma->virt_addr >= (unsigned long)(user_va + size) || vma->virt_addr + vma->area_size <= (unsigned long)user_va))
        {
            return vma;
        }
    }
    return 0;
}

int check_permission(int userId, int VMA_Permission)
{
    switch (PERMISSION_INVAILD(userId, VMA_Permission))
    {
        DUMP_NAME(1, "Read is Invaild in This Vitural Memory Area")
        DUMP_NAME(2, "Write is Invaild in This Vitural Memory Area")
        DUMP_NAME(3, "Write & Read is Invaild in This Vitural Memory Area")
        DUMP_NAME(4, "Exec is Invaild in This Vitural Memory Area")
        DUMP_NAME(5, "Exec & Read is Invaild in This Vitural Memory Area")
        DUMP_NAME(6, "Exec & Write is Invaild in This Vitural Memory Area")
        DUMP_NAME(7, "Exec & Write & Read is Invaild in This Vitural Memory Area")
    default:
        uart_sendlinek("[other Fault]: UNKNOW FAULT");
        break;
    }
    return PERMISSION_INVAILD(userId, VMA_Permission);
}

void dump_vma()
{
    uart_sendlinek("      +--------------------------+\n");
    uart_sendlinek("      | DUMP Vitural Memory Area |\n");
    uart_sendlinek("      +--------------------------+\n");
    list_head_t *pos;
    list_for_each(pos, &curr_thread->vma_list)
    {
        uart_sendlinek("===============================================\n");
        vm_area_struct_t *vma_ptr = (vm_area_struct_t *)pos;

        uart_sendlinek("Vitural Memory Area Name : ");
        switch (vma_ptr->name)
        {
            DUMP_NAME(UNKNOW_AREA, "UNKNOW_AREA")
            DUMP_NAME(USER_DATA, "USER_DATA")
            DUMP_NAME(USER_STACK, "USER_STACK")
            DUMP_NAME(PERIPHERAL, "PERIPHERAL")
            DUMP_NAME(USER_SIGNAL_WRAPPER, "USER_SIGNAL_WRAPPER")
            DUMP_NAME(USER_EXEC_WRAPPER, "USER_EXEC_WRAPPER")
        default:
            uart_sendlinek("unnamed: %d\n", vma_ptr->name);
            break;
        }
        uart_sendlinek("Base Vitural  Address    : 0x%x\n", ((vm_area_struct_t *)pos)->virt_addr);
        uart_sendlinek("Base Physical Address    : 0x%x\n", ((vm_area_struct_t *)pos)->phys_addr);
        uart_sendlinek("Area Size : 0x%x\n", ((vm_area_struct_t *)pos)->area_size);
        uart_sendlinek("Exec, Write, Read : 0x%x\n", ((vm_area_struct_t *)pos)->rwx);
        uart_sendlinek("===============================================\n\n");
    }
}

void dump_pagetable(unsigned long user_va, unsigned long pa)
{
    uart_sendlinek("      +---------------------------+\n");
    uart_sendlinek("      | DUMP PAGE TABLE & ADDRESS |\n");
    uart_sendlinek("      +---------------------------+\n");
    unsigned long *pagetable_pa = curr_thread->context.pgd;
    unsigned long *pagetable_kernel_va = (unsigned long *)PHYS_TO_KERNEL_VIRT(pagetable_pa);

    uart_sendlinek("===============================================\n");
    uart_sendlinek("User Physical Address : 0x%x\n", pa);
    uart_sendlinek("User Vitural  Address : 0x%x\n", user_va);
    unsigned long offset = user_va & 0xFFF;
    uart_sendlinek("Vitural Address offset: 0x%x\n", offset);
    for (int level = 0; level < 4; level++)
    {
        uart_sendlinek("-----------------------------------------------\n");
        uart_sendlinek("PAGE TABLE : ");
        switch (level)
        {
            DUMP_NAME(PGD, "PGD")
            DUMP_NAME(PUD, "PUD")
            DUMP_NAME(PMD, "PMD")
            DUMP_NAME(PTE, "PTE")

        default:
            uart_sendlinek("unnamed: %d\n", level);
            break;
        }
        uart_sendlinek("Pagetable Physical Address: 0x%x\n", pagetable_pa);
        uart_sendlinek("Pagetable Vitural  Address: 0x%x\n", pagetable_kernel_va);

        unsigned int idx = (user_va >> (39 - level * 9)) & 0x1FF;
        uart_sendlinek("Index of Pagetable: 0x%x\n", idx);
        uart_sendlinek("Entry %x of Pagetable: 0x%x\n", idx, pagetable_kernel_va[idx]);
        if (level == PTE)
        {
            pagetable_pa = (unsigned long *)(pagetable_kernel_va[idx] & ENTRY_ADDR_MASK);
            uart_sendlinek("The Page Base Physical Address: 0x%x\n", pagetable_pa);
            uart_sendlinek("The Physical Address: 0x%x\n", (unsigned long)pagetable_pa | offset);
        }
        else
        {
            pagetable_pa = (unsigned long *)(pagetable_kernel_va[idx] & ENTRY_ADDR_MASK);
            uart_sendlinek("next Pagetable Physical Address: 0x%x\n", pagetable_pa);
            pagetable_kernel_va = (unsigned long *)PHYS_TO_KERNEL_VIRT(pagetable_pa);
            uart_sendlinek("next Pagetable Vitural  Address: 0x%x\n", pagetable_kernel_va);
        }
        uart_sendlinek("-----------------------------------------------\n");
    }
}