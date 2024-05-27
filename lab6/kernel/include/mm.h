#ifndef __MM_H
#define __MM_H

#include "allocator.h"

#define PD_TABLE 0b11
#define PD_PAGE 0b11
#define PD_INVALID 0b0
#define PD_ACCESS (1 << 10)
#define PTE_ATTR (PD_ACCESS | (1 << 6) | (1 << 2) | PD_PAGE)

enum vm_type
{
    CODE,
    DATA,
    STACK,
    IO
};

struct mm_struct
{
    struct vm_area_struct *mmap; // list of VMA
    unsigned long long *pgd;     // pgd page table
};

struct vm_area_struct
{
    enum vm_type vm_type;
    unsigned long long vm_start;
    unsigned long long vm_end;
    struct vm_area_struct *vm_next;
};

void init_mm_struct(struct mm_struct *mm_struct);
void mmap_push(struct vm_area_struct **mmap_ptr, enum vm_type vm_type, unsigned long long vm_start, unsigned long long vm_end);
void mappages(struct mm_struct *mm_struct, enum vm_type vm_type, unsigned long long v_add, unsigned long long p_add, unsigned long long size);
unsigned long long *create_page_table();
unsigned long long translate_v_to_p(unsigned long long *pgd, unsigned long long v_add);

#endif