#ifndef __MM_H
#define __MM_H

#include "allocator.h"

#define PD_TABLE 0b11
#define PD_PAGE 0b11
#define PD_INVALID 0b0
#define PD_ACCESS (1 << 10)
#define PD_READ_WRITE (0 << 7)
#define PD_READ_ONLY (1 << 7)
#define PD_ONLY_KERNEL_ACCESS (0 << 6)
#define PD_KERNEL_USER_ACCESS (1 << 6)
#define PTE_ATTR (PD_ACCESS | (1 << 2) | PD_PAGE)

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

#define MAP_ANONYMOUS 0

enum vm_type
{
    CODE,
    DATA,
    STACK,
    IO,
    SYSCALL
};

struct mm_struct
{
    struct vm_area_struct *mmap; // list of VMA
    unsigned long long *pgd;     // pgd page table
};

struct vm_area_struct
{
    enum vm_type vm_type;                     // section type
    unsigned long long pm_base;               // physical address base
    unsigned long long vm_start;              // virtual address start
    unsigned long long vm_end;                // virtual address end
    struct vm_area_struct *vm_prev, *vm_next; // point to next vm_area_struct
    int vm_page_prot;                         // access permission of VMA
    int vm_flags;
};

void init_mm_struct(struct mm_struct *mm_struct);
void mmap_push(struct vm_area_struct **mmap_ptr, enum vm_type vm_type, unsigned long long pm_base, unsigned long long vm_start, unsigned long long vm_end, int prot, int flags);
void mappages(struct mm_struct *mm_struct, enum vm_type vm_type, unsigned long long v_add, unsigned long long p_add, unsigned long long size, int prot, int flags);

unsigned long long *create_page_table();
void page_reclaim(unsigned long long *pgd);
void page_reclaim_pud(unsigned long long *pud);
void page_reclaim_pmd(unsigned long long *pmd);

void change_all_pgd_prot(unsigned long long *pgd, int prot);
void change_all_pud_prot(unsigned long long *pud, int prot);
void change_all_pmd_prot(unsigned long long *pmd, int prot);
void change_all_pte_prot(unsigned long long *pte, int prot);

void copy_pgd_table(unsigned long long *pgd, unsigned long long *copy_pgd);
void copy_pud_table(unsigned long long *pud, unsigned long long *copy_pud);
void copy_pmd_table(unsigned long long *pmd, unsigned long long *copy_pmd);
void copy_pte_table(unsigned long long *pte, unsigned long long *copy_pte);

unsigned long long translate_v_to_p(unsigned long long *pgd, unsigned long long v_add);

void do_translation_fault(unsigned long long address);
void do_permission_fault(unsigned long long address);
void do_page_fault(unsigned long long p_add, unsigned long long v_add, int prot);
void do_bad_area(unsigned long long address);

#endif