#include "uart.h"
#include "exception.h"
#include "mmu.h"
#include "mm.h"

void init_mm_struct(struct mm_struct *mm_struct)
{
    mm_struct->mmap = NULL;
    mm_struct->pgd = (unsigned long long *)((unsigned long long)create_page_table() & ~VA_START); // save pgd to physical address
}

void mmap_push(struct vm_area_struct **mmap_ptr, enum vm_type vm_type, unsigned long long vm_start, unsigned long long vm_end)
{
    disable_interrupt();
    struct vm_area_struct *new_item = (struct vm_area_struct *)kmalloc(sizeof(struct vm_area_struct));
    new_item->vm_type = vm_type;
    new_item->vm_start = vm_start;
    new_item->vm_end = vm_end;
    new_item->vm_next = NULL;

    if (*mmap_ptr == NULL)
        *mmap_ptr = new_item;
    else
    {
        struct vm_area_struct *cur = *mmap_ptr;
        while (cur->vm_next != NULL)
            cur = cur->vm_next;
        cur->vm_next = new_item;
    }
}

void mappages(struct mm_struct *mm_struct, enum vm_type vm_type, unsigned long long v_add, unsigned long long p_add, unsigned long long size)
{
    unsigned long long cur_size = 0;
    mmap_push(&(mm_struct->mmap), vm_type, p_add, p_add + size);

    while (cur_size <= size)
    {
        unsigned long long *cur_table = (unsigned long long *)(VA_START | (unsigned long long)(mm_struct->pgd)); // map pdg to kenel sacpe

        for (int level = 0; level < 4; level++)
        {
            unsigned long long idx = (v_add >> (12 + 9 * (3 - level))) & 0x1ff;
            unsigned long long *pte = &cur_table[idx];

            if (*pte == PD_INVALID) // page table entry is invalid
            {
                if (level == 3)
                {
                    *pte = (p_add & ((unsigned long long)(0xfffffffff) << 12)) | PTE_ATTR; // descriptor address [47:12]
                }
                else
                {
                    unsigned long long *next_level_table = create_page_table();
                    *pte = ((unsigned long long)next_level_table & ~VA_START) | PD_TABLE; // map to physical address
                    cur_table = next_level_table;
                }
            }
            else
            {
                if (level == 3)
                {
                    *pte = (p_add & ((unsigned long long)(0xfffffffff) << 12)) | PTE_ATTR; // descriptor address [47:12]
                }
                else
                {
                    unsigned long long next_level_table_address = *pte & ((unsigned long long)(0xfffffffff) << 12); // descriptor address [47:12]
                    cur_table = (unsigned long long *)(VA_START | next_level_table_address);
                }
            }
        }

        v_add += 0x1000;
        p_add += 0x1000;
        cur_size += 0x1000;
    }
}

unsigned long long *create_page_table()
{
    disable_interrupt();
    unsigned long long *page_table_head = (unsigned long long *)kmalloc(4096);

    for (int i = 0; i < 512; i++) // set all the entrys to invalid
        page_table_head[i] = PD_INVALID;

    return page_table_head;
}

unsigned long long translate_v_to_p(unsigned long long *pgd, unsigned long long v_add)
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