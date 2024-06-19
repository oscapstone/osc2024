#include "vm.h"
#include "mm.h"
#include "string.h"

static void walk(unsigned long pt, unsigned long va, unsigned long pa,
                 unsigned long prot)
{
    unsigned long *table = (unsigned long *)pt;
    for (int level = 0; level <= 3; level++) {
        unsigned long offset = (va >> (39 - 9 * level)) & 0x1FF;
        if (level == 3) {
            table[offset] = pa | PE_NORMAL_ATTR;
            return;
        }
        if (!table[offset]) {
            unsigned long *t = kmalloc(PAGE_SIZE);
            memset(t, 0, PAGE_SIZE);
            table[offset] = VIRT_TO_PHYS((unsigned long)t) | PD_TABLE;
        }
        table = (unsigned long *)PHYS_TO_VIRT((table[offset] & ~0xFFF));
    }
}

void map_pages(unsigned long pgd, unsigned long va, unsigned long size,
               unsigned long pa, unsigned long prot)
{
    for (int i = 0; i < size; i += PAGE_SIZE)
        walk(pgd, va + i, pa + i, prot);
}

void add_vma(struct vm_area_struct **head, unsigned long va, unsigned long pa,
             unsigned long size, unsigned long prot)
{
    struct vm_area_struct *vma = kmalloc(sizeof(struct vm_area_struct));
    vma->va = va & ~(PAGE_SIZE - 1);
    vma->pa = pa & ~(PAGE_SIZE - 1);
    vma->size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    vma->prot = prot;
    vma->prev = 0;
    vma->next = 0;

    if (*head == 0 || vma->va < (*head)->va) {
        vma->next = *head;
        if (*head != 0)
            (*head)->prev = vma;
        *head = vma;
        return;
    }

    struct vm_area_struct *current = *head;
    while (current->next != 0 && current->next->va <= vma->va)
        current = current->next;
    vma->next = current->next;
    if (current->next != 0)
        current->next->prev = vma;
    current->next = vma;
    vma->prev = current;
}

void del_vma(struct vm_area_struct **head, unsigned long va)
{
    struct vm_area_struct *current = *head;
    while (current != 0 && current->va != va)
        current = current->next;
    if (current == 0)
        return;
    if (current->prev != 0)
        current->prev->next = current->next;
    if (current->next != 0)
        current->next->prev = current->prev;
    if (current == *head)
        *head = current->next;
    kfree(current);
}

void copy_vmas(struct vm_area_struct *from, struct vm_area_struct **to)
{
}

void page_fault_handler()
{
}
