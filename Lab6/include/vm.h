#ifndef __VM_H__
#define __VM_H__

#define VIRT_OFFSET 0xFFFF000000000000

typedef unsigned long pte_t;
typedef pte_t* pagetable_t;

typedef struct vm_area{
  unsigned long va;
  unsigned long pa;
  unsigned long sz;
  unsigned long flags;
  int ref_count;
  struct vm_area *prev;
  struct vm_area *next;
}vm_area_t;

unsigned long virt_to_phys(void* vaddr);
unsigned long phys_to_virt(void* paddr);

void add_vma(vm_area_t **head, unsigned long va, unsigned long pa, unsigned long sz, unsigned long attr);
void remove_vma(vm_area_t **head, unsigned long va);
void clear_vma_list(vm_area_t **head);
void copy_vma_list(vm_area_t **dst, vm_area_t *src);
void dump_vma_list(vm_area_t *head);

pte_t* walk(pagetable_t pgt, unsigned long vaddr, int alloc);
int map_pages(pagetable_t pgt, unsigned long va, unsigned long pa, unsigned long sz, unsigned long attr);
void copy_pagetable(pagetable_t dst, pagetable_t src, int level);
void clear_pagetable(pagetable_t pgt, int level);
void dump_pagetable(pagetable_t pgt, int level);

void page_fault_handler();

#endif