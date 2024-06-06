#ifndef _VM_H
#define _VM_H

#include "types.h"
#include "vm_macro.h"

extern char __pt_start;

#define KER_PGD_ADDR ((uint64_t)(&__pt_start) ^ KERNEL_VIRT_BASE)

#define VM_PROT_NONE 0
#define VM_PROT_READ 1
#define VM_PROT_WRITE 2
#define VM_PROT_EXEC 4

#define MAP_ANONYMOUS 0x20
#define MAP_POPULATE 1

enum vm_type { CODE, STACK, IO, DATA };

typedef struct vm_area_struct {
  enum vm_type vm_type;                      // section type
  uint64_t pm_start;                         // physical address base
  uint64_t vm_start;                         // virtual address start
  uint64_t area_sz;                          // address range size
  struct vm_area_struct *vm_prev, *vm_next;  // point to next vm_area_struct
  uint8_t vm_prot;                           // access permission of VMA
  uint8_t vm_flag;                           // type of memory mapped region
} vm_area_struct;

typedef struct mm_struct {
  uint64_t pgd;
  vm_area_struct *mmap_list_head;  // list of VMA
} mm_struct;

typedef struct task_struct task_struct;

uint64_t vir2phy(uint64_t vm_addr);
uint64_t phy2vir(uint64_t pm_addr);
void free_pages(task_struct *thread, uint64_t ker_vm_addr);
void map_pages(task_struct *thread, enum vm_type vmtype, uint64_t ker_vm_addr,
               uint64_t vm_addr, uint32_t size, uint8_t vm_prot,
               uint8_t vm_flag);
void translation_fault_handler(uint64_t vm_addr);
void permission_fault_handler(uint64_t vm_addr);
void vm_free(task_struct *thread);
void change_all_page_prot(uint64_t *pt, uint32_t level, uint8_t vm_prot);
void copy_all_pt(uint64_t *pt_dst, uint64_t *pt_src, uint32_t level);

#endif