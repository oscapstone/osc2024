#include "uart.h"
#include "exception.h"
#include "schedule.h"
#include "mmu.h"
#include "mm.h"

void init_mm_struct(struct mm_struct *mm_struct)
{
    mm_struct->mmap = NULL;
    mm_struct->pgd = (unsigned long long *)((unsigned long long)create_page_table() & ~VA_START); // save pgd to physical address
}

void mmap_push(struct vm_area_struct **mmap_ptr, enum vm_type vm_type, unsigned long long pm_base, unsigned long long vm_start, unsigned long long vm_end, int prot, int flags)
{
    disable_interrupt();
    struct vm_area_struct *new_item = (struct vm_area_struct *)kmalloc(sizeof(struct vm_area_struct));
    new_item->vm_type = vm_type;
    new_item->pm_base = pm_base;
    new_item->vm_start = vm_start;
    new_item->vm_end = vm_end;
    new_item->vm_page_prot = prot;
    new_item->vm_flags = flags;
    new_item->vm_prev = NULL;
    new_item->vm_next = NULL;

    if (*mmap_ptr == NULL) // if mmap's head is empty, pointer it to new item
        *mmap_ptr = new_item;
    else // find the tail of list, and insert to it.
    {
        struct vm_area_struct *cur = *mmap_ptr;
        while (cur->vm_next != NULL)
            cur = cur->vm_next;

        cur->vm_next = new_item;
        new_item->vm_prev = cur;
    }
}

void mappages(struct mm_struct *mm_struct, enum vm_type vm_type, unsigned long long v_add, unsigned long long p_add, unsigned long long size, int prot, int flags)
{
    mmap_push(&(mm_struct->mmap), vm_type, p_add, v_add, v_add + size, prot, flags); // just push it to VMA list for demand paging
}

unsigned long long *create_page_table()
{
    disable_interrupt();
    unsigned long long *page_table_head = (unsigned long long *)kmalloc(4096);

    for (int i = 0; i < 512; i++) // set all the entrys to invalid
        page_table_head[i] = PD_INVALID;

    return page_table_head;
}

void page_reclaim(unsigned long long *pgd) // reclaim all the pages
{
    pgd = (unsigned long long *)((unsigned long long)pgd | VA_START);
    for (int i = 0; i < 512; i++)
    {
        if (pgd[i] != PD_INVALID)
            page_reclaim_pud((unsigned long long *)((pgd[i] & ((unsigned long long)(0xfffffffff) << 12)) | VA_START));
    }
    kfree(pgd);

    switch_mm_irqs_off(get_current_task()->mm_struct->pgd); // flush tlb and pipeline because page table is change
    disable_interrupt();
}

void page_reclaim_pud(unsigned long long *pud)
{
    for (int i = 0; i < 512; i++)
    {
        if (pud[i] != PD_INVALID)
            page_reclaim_pmd((unsigned long long *)((pud[i] & ((unsigned long long)(0xfffffffff) << 12)) | VA_START));
    }
    kfree(pud);
}

void page_reclaim_pmd(unsigned long long *pmd)
{
    for (int i = 0; i < 512; i++)
    {
        if (pmd[i] != PD_INVALID)
            kfree((unsigned long long *)((pmd[i] & ((unsigned long long)(0xfffffffff) << 12)) | VA_START));
    }
    kfree(pmd);
}

void change_all_pgd_prot(unsigned long long *pgd, int prot)
{
    pgd = (unsigned long long *)((unsigned long long)(pgd) | VA_START);
    for (int i = 0; i < 512; i++)
    {
        if (pgd[i] != PD_INVALID)
        {
            unsigned long long *pud = (unsigned long long *)((pgd[i] & ((unsigned long long)(0xfffffffff) << 12)) | VA_START);
            change_all_pud_prot(pud, prot);
        }
    }

    switch_mm_irqs_off(get_current_task()->mm_struct->pgd); // flush tlb and pipeline because page table is change
    disable_interrupt();
}

void change_all_pud_prot(unsigned long long *pud, int prot)
{
    for (int i = 0; i < 512; i++)
    {
        if (pud[i] != PD_INVALID)
        {
            unsigned long long *pmd = (unsigned long long *)((pud[i] & ((unsigned long long)(0xfffffffff) << 12)) | VA_START);
            change_all_pmd_prot(pmd, prot);
        }
    }
}

void change_all_pmd_prot(unsigned long long *pmd, int prot)
{
    for (int i = 0; i < 512; i++)
    {
        if (pmd[i] != PD_INVALID)
        {
            unsigned long long *pte = (unsigned long long *)((pmd[i] & ((unsigned long long)(0xfffffffff) << 12)) | VA_START);
            change_all_pte_prot(pte, prot);
        }
    }
}

void change_all_pte_prot(unsigned long long *pte, int prot)
{
    for (int i = 0; i < 512; i++)
    {
        if (pte[i] != PD_INVALID)
        {
            pte[i] &= ~(0b1 << 7); // clean the bit[7]
            if ((prot & 0b11) == (PROT_READ | PROT_WRITE))
                pte[i] |= PD_READ_WRITE;
            else if ((prot & 0b11) == PROT_READ)
                pte[i] |= PD_READ_ONLY;
        }
    }
}

void copy_pgd_table(unsigned long long *pgd, unsigned long long *copy_pgd)
{
    pgd = (unsigned long long *)((unsigned long long)(pgd) | VA_START);
    copy_pgd = (unsigned long long *)((unsigned long long)(copy_pgd) | VA_START);
    for (int i = 0; i < 512; i++)
    {
        if (pgd[i] != PD_INVALID)
        {
            unsigned long long *pud = (unsigned long long *)((pgd[i] & ((unsigned long long)(0xfffffffff) << 12)) | VA_START);
            unsigned long long *copy_pud = create_page_table();
            copy_pgd[i] = ((unsigned long long)copy_pud & ~VA_START) | PD_TABLE; // map to physical address
            copy_pud_table(pud, copy_pud);
        }
        else
            copy_pgd[i] = PD_INVALID;
    }
}

void copy_pud_table(unsigned long long *pud, unsigned long long *copy_pud)
{
    for (int i = 0; i < 512; i++)
    {
        if (pud[i] != PD_INVALID)
        {
            unsigned long long *pmd = (unsigned long long *)((pud[i] & ((unsigned long long)(0xfffffffff) << 12)) | VA_START);
            unsigned long long *copy_pmd = create_page_table();
            copy_pud[i] = ((unsigned long long)copy_pmd & ~VA_START) | PD_TABLE; // map to physical address
            copy_pmd_table(pmd, copy_pmd);
        }
        else
            copy_pud[i] = PD_INVALID;
    }
}

void copy_pmd_table(unsigned long long *pmd, unsigned long long *copy_pmd)
{
    for (int i = 0; i < 512; i++)
    {
        if (pmd[i] != PD_INVALID)
        {
            unsigned long long *pte = (unsigned long long *)((pmd[i] & ((unsigned long long)(0xfffffffff) << 12)) | VA_START);
            unsigned long long *copy_pte = create_page_table();
            copy_pmd[i] = ((unsigned long long)copy_pte & ~VA_START) | PD_TABLE; // map to physical address

            for (int j = 0; j < 512; j++)
                copy_pte[j] = pte[j];
        }
        else
            copy_pmd[i] = PD_INVALID;
    }
}

unsigned long long translate_v_to_p(unsigned long long *pgd, unsigned long long v_add) // translate the virtual addrss to physical address through pgd
{
    unsigned long long *cur_table = (unsigned long long *)(VA_START | (unsigned long long)pgd); // map pdg to kenel sacpe

    for (int level = 0; level < 4; level++)
    {
        unsigned long long idx = (v_add >> (12 + 9 * (3 - level))) & 0x1ff;
        unsigned long long *pte = &cur_table[idx];

        if (level < 3)
        {
            unsigned long long next_level_table_address = *pte & ((unsigned long long)(0xfffffffff) << 12); // descriptor address [47:12]
            cur_table = (unsigned long long *)(VA_START | next_level_table_address);
        }
        else
            return (*pte & ((unsigned long long)(0xfffffffff) << 12)) | (v_add & 0xfff);
    }

    return 0;
}

void do_translation_fault(unsigned long long address)
{
    struct task_struct *cur_task = get_current_task();

    for (struct vm_area_struct *cur_vma = cur_task->mm_struct->mmap; cur_vma != NULL; cur_vma = cur_vma->vm_next)
    {
        if (address >= cur_vma->vm_start && address <= cur_vma->vm_end) // try to search address in VMA list
        {
            unsigned long long offset = address - cur_vma->vm_start;
            unsigned long long p_add = cur_vma->pm_base + offset;
            do_page_fault(p_add, address, cur_vma->vm_page_prot);
            return;
        }
    }

    do_bad_area(address); // the address is not find, try to kill the process.
}

void do_permission_fault(unsigned long long address)
{
    uart_puts("[Permission fault]: 0x");
    uart_hex_lower_case(address);
    uart_puts("\n");

    struct task_struct *cur_task = get_current_task();

    for (struct vm_area_struct *cur_vma = cur_task->mm_struct->mmap; cur_vma != NULL; cur_vma = cur_vma->vm_next)
    {
        if (address >= cur_vma->vm_start && address <= cur_vma->vm_end) // try to search address in VMA list
        {
            if ((cur_vma->vm_page_prot & 0b11) == (PROT_READ | PROT_WRITE))
            {
                unsigned long long v_add = address;
                unsigned long long p_add = VA_START | translate_v_to_p(cur_task->mm_struct->pgd, address);

                char *frame = (char *)((p_add >> 12) << 12);
                char *copy_frame = NULL;

                copy_frame = kmalloc(4096); // allocate one page frame and copy from orignal page frame
                for (int i = 0; i < 4096; i++)
                    copy_frame[i] = frame[i];

                unsigned long long *cur_table = (unsigned long long *)(VA_START | (unsigned long long)cur_task->mm_struct->pgd); // map pdg to kenel sacpe
                for (int level = 0; level < 4; level++)
                {
                    unsigned long long idx = (v_add >> (12 + 9 * (3 - level))) & 0x1ff;
                    unsigned long long *pte = &cur_table[idx];

                    if (level == 3)
                        // change the pte's bit[7] from read only to read/write
                        *pte = ((unsigned long long)copy_frame - VA_START) | PD_READ_WRITE | PD_KERNEL_USER_ACCESS | PTE_ATTR;
                    else
                    {
                        unsigned long long next_level_table_address = *pte & ((unsigned long long)(0xfffffffff) << 12); // descriptor address [47:12]
                        cur_table = (unsigned long long *)(VA_START | next_level_table_address);
                    }
                }
                break;
            }
            else
                do_bad_area(address); // the address is not find, try to kill the process.
        }
    }

    switch_mm_irqs_off(cur_task->mm_struct->pgd); // flush tlb and pipeline because page table is change
    disable_interrupt();
}

void do_page_fault(unsigned long long p_add, unsigned long long v_add, int prot)
{
    uart_puts("[Translation fault]: 0x");
    uart_hex_lower_case(v_add);
    uart_puts("\n");

    unsigned long long *cur_table = (unsigned long long *)(VA_START | (unsigned long long)(get_current_task()->mm_struct->pgd)); // map pdg to kenel sacpe

    for (int level = 0; level < 4; level++)
    {
        unsigned long long idx = (v_add >> (12 + 9 * (3 - level))) & 0x1ff; // find the index of next level page entry from virtual address
        unsigned long long *pte = &cur_table[idx];

        if (*pte == PD_INVALID) // page table entry is invalid
        {
            if (level == 3)
            {
                if ((prot & 0b11) == (PROT_READ | PROT_WRITE))
                    *pte = (p_add & ((unsigned long long)(0xfffffffff) << 12)) | PD_READ_WRITE | PD_KERNEL_USER_ACCESS | PTE_ATTR; // descriptor address [47:12]
                else if ((prot & 0b11) == PROT_READ)
                    *pte = (p_add & ((unsigned long long)(0xfffffffff) << 12)) | PD_READ_ONLY | PD_KERNEL_USER_ACCESS | PTE_ATTR; // descriptor address [47:12]
                else
                    *pte = (p_add & ((unsigned long long)(0xfffffffff) << 12)) | PTE_ATTR; // descriptor address [47:12]
            }
            else
            {
                unsigned long long *next_level_table = create_page_table();
                *pte = ((unsigned long long)next_level_table & ~VA_START) | PD_TABLE; // map to physical address
                cur_table = next_level_table;
            }
        }
        else // page table entry is valid, try to find the next level page entry or revise it.
        {
            if (level == 3)
            {
                if ((prot & 0b11) == (PROT_READ | PROT_WRITE))
                    *pte = (p_add & ((unsigned long long)(0xfffffffff) << 12)) | PD_READ_WRITE | PD_KERNEL_USER_ACCESS | PTE_ATTR; // descriptor address [47:12]
                else if ((prot & 0b11) == PROT_READ)
                    *pte = (p_add & ((unsigned long long)(0xfffffffff) << 12)) | PD_READ_ONLY | PD_KERNEL_USER_ACCESS | PTE_ATTR; // descriptor address [47:12]
                else
                    *pte = (p_add & ((unsigned long long)(0xfffffffff) << 12)) | PTE_ATTR; // descriptor address [47:12]
            }
            else
            {
                unsigned long long next_level_table_address = *pte & ((unsigned long long)(0xfffffffff) << 12); // descriptor address [47:12]
                cur_table = (unsigned long long *)(VA_START | next_level_table_address);
            }
        }
    }

    switch_mm_irqs_off(get_current_task()->mm_struct->pgd); // flush tlb and pipeline because page table is change
    disable_interrupt();
}

void do_bad_area(unsigned long long address)
{
    uart_puts("[Segmentation fault]: 0x");
    uart_hex_lower_case(address);
    uart_puts(", Kill Process\n");
    task_exit();
}