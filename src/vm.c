#include "vm.h"

#include "interrupt.h"
#include "mem.h"
#include "multitask.h"
#include "string.h"
#include "uart1.h"
#include "utli.h"
#include "vm_macro.h"

extern frame_entry *frame_entry_arr;

uint64_t vir2phy(uint64_t vm_addr) { return (vm_addr << 16) >> 16; }

uint64_t phy2vir(uint64_t pm_addr) { return (pm_addr | KERNEL_VIRT_BASE); }

static void create_pgd(task_struct *thread) {
  if (thread->mm.pgd == KER_PGD_ADDR) {
    void *addr = malloc(PAGE_SIZE);
    memset(addr, PD_INVAILD, PAGE_SIZE);
    thread->mm.pgd = vir2phy((uint64_t)addr);
  }
}

static uint64_t *get_pgd(task_struct *thread) {
  return (uint64_t *)phy2vir(thread->mm.pgd);
}

static uint64_t *create_pt(uint64_t *prev_table, uint32_t idx) {
  if (prev_table[idx] == 0) {
    void *addr = malloc(PAGE_SIZE);
    memset(addr, PD_INVAILD, PAGE_SIZE);
    prev_table[idx] = (vir2phy((uint64_t)addr) | PD_TABLE);
  }
  return (uint64_t *)phy2vir((prev_table[idx] & PAGE_MASK));
}

static uint64_t *get_pt(uint64_t *pt, uint32_t idx) {
  return (uint64_t *)phy2vir((pt[idx] & PAGE_MASK));
}

static void mmap_push_front(vm_area_struct **mmap_list_head,
                            enum vm_type vmtype, uint64_t pm_start,
                            uint64_t vm_start, uint32_t area_sz,
                            uint8_t vm_prot, uint8_t vm_flag) {
  vm_area_struct *new_node = (vm_area_struct *)malloc(sizeof(vm_area_struct));
  new_node->vm_type = vmtype;
  new_node->pm_start = pm_start;
  new_node->vm_start = vm_start;
  new_node->area_sz = area_sz;
  new_node->vm_prot = vm_prot;
  new_node->vm_flag = vm_flag;
  new_node->vm_next = *mmap_list_head;
  *mmap_list_head = new_node;
}

static void invaildate_page(uint64_t *pgd, uint64_t vm_addr) {
  uint32_t pgd_idx = (vm_addr & (PD_MASK << PGD_SHIFT)) >> PGD_SHIFT;
  uint32_t pud_idx = (vm_addr & (PD_MASK << PUD_SHIFT)) >> PUD_SHIFT;
  uint32_t pmd_idx = (vm_addr & (PD_MASK << PMD_SHIFT)) >> PMD_SHIFT;
  uint32_t pte_idx = (vm_addr & (PD_MASK << PTE_SHIFT)) >> PTE_SHIFT;

  uint64_t *pud = create_pt(pgd, pgd_idx);
  uint64_t *pmd = create_pt(pud, pud_idx);
  uint64_t *pte = create_pt(pmd, pmd_idx);

  pte[pte_idx] = PD_INVAILD;
}

void free_pages(task_struct *thread, uint64_t ker_vm_addr) {
  vm_area_struct *node = thread->mm.mmap_list_head;
  uint64_t vm_addr;
  uint32_t size;
  if (phy2vir(node->pm_start) == ker_vm_addr) {
    size = node->area_sz;
    vm_addr = node->vm_start;
    thread->mm.mmap_list_head = node->vm_next;
  } else {
    while (phy2vir(node->vm_next->pm_start) != ker_vm_addr) {
      node = node->vm_next;
    }
    size = node->vm_next->area_sz;
    vm_addr = node->vm_start;
    node->vm_next = node->vm_next->vm_next;
  }

  frame_entry_arr[address2idx((void *)ker_vm_addr)].ref_cnt--;
  if (frame_entry_arr[address2idx((void *)ker_vm_addr)].ref_cnt == 0) {
    free((void *)ker_vm_addr);
  }

  uint32_t page_num = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  for (uint32_t i = 0; i < page_num; i++) {
    invaildate_page((uint64_t *)thread->mm.pgd, vm_addr + i * PAGE_SIZE);
  }
}

void map_pages(task_struct *thread, enum vm_type vmtype, uint64_t ker_vm_addr,
               uint64_t vm_addr, uint32_t size, uint8_t vm_prot,
               uint8_t vm_flag) {
  create_pgd(thread);

  mmap_push_front(&thread->mm.mmap_list_head, vmtype, vir2phy(ker_vm_addr),
                  vm_addr, size, vm_prot, vm_flag);

  if (vmtype != IO) {
    frame_entry_arr[address2idx((void *)ker_vm_addr)].ref_cnt++;
  }
}

static void fill_pte_entry(uint64_t *pte, uint32_t pte_idx, uint64_t pm_addr,
                           uint8_t vm_prot) {
  if (vm_prot & VM_PROT_WRITE) {
    pte[pte_idx] = ((pm_addr & PAGE_MASK) | PTE_NORAL_ATTR | PD_USER_RW_FLAG);
  } else {
    pte[pte_idx] = ((pm_addr & PAGE_MASK) | PTE_NORAL_ATTR | PD_USER_RO_FLAG);
  }
}

static void segmentation_fault_handler(uint64_t vm_addr) {
  uart_send_string("[Segmentation fault]: ");
  uart_hex_64(vm_addr);
  uart_puts(", Kill Process");
  task_exit();
}

void translation_fault_handler(uint64_t vm_addr) {
  uart_send_string("\r\n[Translation fault]: ");
  uart_hex_64(vm_addr);
  uart_send_string("\r\n");

  uint32_t pgd_idx = (vm_addr & (PD_MASK << PGD_SHIFT)) >> PGD_SHIFT;
  uint32_t pud_idx = (vm_addr & (PD_MASK << PUD_SHIFT)) >> PUD_SHIFT;
  uint32_t pmd_idx = (vm_addr & (PD_MASK << PMD_SHIFT)) >> PMD_SHIFT;
  uint32_t pte_idx = (vm_addr & (PD_MASK << PTE_SHIFT)) >> PTE_SHIFT;

  uint64_t *pgd = get_pgd(current_thread);
  uint64_t *pud = create_pt(pgd, pgd_idx);
  uint64_t *pmd = create_pt(pud, pud_idx);
  uint64_t *pte = create_pt(pmd, pmd_idx);

  for (vm_area_struct *ptr = current_thread->mm.mmap_list_head;
       ptr != (vm_area_struct *)0; ptr = ptr->vm_next) {
    if (vm_addr >= ptr->vm_start && vm_addr <= ptr->vm_start + ptr->area_sz) {
      uint64_t pm_addr = ptr->pm_start + (vm_addr - ptr->vm_start);
      fill_pte_entry(pte, pte_idx, pm_addr, ptr->vm_prot);
      return;
    }
  }
  segmentation_fault_handler(vm_addr);
}

void permission_fault_handler(uint64_t vm_addr) {
  uart_send_string("\r\n[Permission fault]: ");
  uart_hex_64(vm_addr);
  uart_send_string("\r\n");

  uint32_t pgd_idx = (vm_addr & (PD_MASK << PGD_SHIFT)) >> PGD_SHIFT;
  uint32_t pud_idx = (vm_addr & (PD_MASK << PUD_SHIFT)) >> PUD_SHIFT;
  uint32_t pmd_idx = (vm_addr & (PD_MASK << PMD_SHIFT)) >> PMD_SHIFT;
  uint32_t pte_idx = (vm_addr & (PD_MASK << PTE_SHIFT)) >> PTE_SHIFT;

  uint64_t *pgd = get_pgd(current_thread);
  uint64_t *pud = get_pt(pgd, pgd_idx);
  uint64_t *pmd = get_pt(pud, pud_idx);
  uint64_t *pte = get_pt(pmd, pmd_idx);

  for (vm_area_struct *ptr = current_thread->mm.mmap_list_head;
       ptr != (vm_area_struct *)0; ptr = ptr->vm_next) {
    if (vm_addr >= ptr->vm_start && vm_addr <= ptr->vm_start + ptr->area_sz) {
      uint64_t pm_addr = ptr->pm_start + (vm_addr - ptr->vm_start);
      uint32_t frame_idx = address2idx((void *)phy2vir(ptr->pm_start));

      if (ptr->vm_type == CODE) {
        ptr->vm_prot |= VM_PROT_EXEC;
      } else if (ptr->vm_type == STACK || ptr->vm_type == DATA) {
        ptr->vm_prot |= VM_PROT_WRITE;
      } else {
        segmentation_fault_handler(vm_addr);
      }

      if (frame_entry_arr[frame_idx].ref_cnt >
          1) {  // At least two threads use this area in the same time
        frame_entry_arr[frame_idx].ref_cnt--;
        void *addr = malloc(ptr->area_sz);
        frame_entry_arr[address2idx(addr)].ref_cnt = 1;
        memcpy(addr, (void *)phy2vir(ptr->pm_start), ptr->area_sz);
        ptr->pm_start = vir2phy((uint64_t)addr);
        pm_addr = ptr->pm_start + (vm_addr - ptr->vm_start);
      }

      fill_pte_entry(pte, pte_idx, pm_addr, ptr->vm_prot);
      // invalidate all TLB entries after modifying the pte
      asm volatile(
          "dsb ish\n\t"
          "tlbi vmalle1is\n\t"
          "dsb ish\n\t"
          "isb");
      return;
    }
  }
  segmentation_fault_handler(vm_addr);
}

static void free_page_table(uint64_t *pt, uint32_t level) {
  if (level < 3) {
    for (uint32_t i = 0; i < 512; i++) {
      if (pt[i] != PD_INVAILD) {
        free_page_table(get_pt(pt, i), level + 1);
      }
    }
  }
  if (level > 0) {
    free((void *)pt);
  }
}

void vm_free(task_struct *thread) {
  // free vm pages
  vm_area_struct *ptr = thread->mm.mmap_list_head;
  while (ptr != (vm_area_struct *)0) {
    vm_area_struct *next = ptr->vm_next;

    if (ptr->vm_type != IO) {
      uint32_t frame_idx = address2idx((void *)phy2vir(ptr->pm_start));
      frame_entry_arr[frame_idx].ref_cnt--;
      if (frame_entry_arr[frame_idx].ref_cnt ==
          0) {  // if there are no other threads using this area
        memset((void *)phy2vir(ptr->pm_start), 0, ptr->area_sz);
        free((void *)phy2vir(ptr->pm_start));
      }
    }

    free((void *)ptr);
    ptr = next;
  }
  thread->mm.mmap_list_head = (vm_area_struct *)0;

  // free page tables
  uint64_t *pgd = (uint64_t *)phy2vir(thread->mm.pgd);
  free_page_table(pgd, 0);
  memset((void *)pgd, PD_INVAILD, PAGE_SIZE);
}

void change_all_page_prot(uint64_t *pt, uint32_t level, uint8_t vm_prot) {
  for (uint32_t i = 0; i < 512; i++) {
    if (pt[i] != PD_INVAILD) {
      if (level < 3) {
        change_all_page_prot(get_pt(pt, i), level + 1, vm_prot);
      } else {
        pt[i] &= ~(0b11 << 6);  // clean the bit[7]
        if (vm_prot & VM_PROT_WRITE) {
          pt[i] |= PD_USER_RW_FLAG;
        } else {
          pt[i] |= PD_USER_RO_FLAG;
        }
      }
    }
  }
}

void copy_all_pt(uint64_t *pt_dst, uint64_t *pt_src, uint32_t level) {
  for (uint32_t i = 0; i < 512; i++) {
    if (pt_src[i] != PD_INVAILD) {
      if (level < 3) {
        copy_all_pt(create_pt(pt_dst, i), get_pt(pt_src, i), level + 1);
      } else {
        pt_dst[i] = pt_src[i];
      }
    }
  }
}