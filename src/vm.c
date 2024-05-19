#include "vm.h"

#include "mem.h"
#include "string.h"
#include "uart1.h"
#include "vm_macro.h"

static uint64_t vir2phy(uint64_t virt_addr) { return (virt_addr << 16) >> 16; }

static uint64_t *create_pgd(task_struct *thread) {
  if (thread->mm.pgd == KER_PGD_ADDR) {
    void *addr = malloc(PAGE_SIZE);
    memset(addr, 0, PAGE_SIZE);
    thread->mm.pgd = vir2phy((uint64_t)addr);
  }
  return (uint64_t *)(thread->mm.pgd | KERNEL_VIRT_BASE);
}

static uint64_t *create_pt(uint64_t *prev_table, uint32_t idx) {
  if (prev_table[idx] == 0) {
    void *addr = malloc(PAGE_SIZE);
    memset(addr, 0, PAGE_SIZE);
    prev_table[idx] = (vir2phy((uint64_t)addr) | PD_TABLE);
  }
  return (uint64_t *)((prev_table[idx] & PAGE_MASK) | KERNEL_VIRT_BASE);
}

static void *create_page(uint64_t *pte, uint32_t idx, uint32_t size) {
  uint32_t page_num = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  void *addr = malloc(page_num * PAGE_SIZE);
  memset(addr, 0, page_num * PAGE_SIZE);
  for (uint32_t i = 0; i < page_num; i++) {
    pte[idx + i] = (vir2phy((uint64_t)addr + i * PAGE_SIZE) | PTE_NORAL_ATTR |
                    PD_USER_RW_FLAG);
  }
  return (void *)((pte[idx] & PAGE_MASK) | KERNEL_VIRT_BASE);
}

void *map_pages(task_struct *thread, uint64_t user_addr, uint32_t size) {
  if (size > (1 << 21)) {
    uart_puts("map_pages: too large size (> 2MB)");
    return (void *)0;
  }

  uint64_t *pgd = create_pgd(thread);

  uint32_t pgd_idx = (user_addr & (PD_MASK << PGD_SHIFT)) >> PGD_SHIFT;
  uint32_t pud_idx = (user_addr & (PD_MASK << PUD_SHIFT)) >> PUD_SHIFT;
  uint32_t pmd_idx = (user_addr & (PD_MASK << PMD_SHIFT)) >> PMD_SHIFT;
  uint32_t pte_idx = (user_addr & (PD_MASK << PTE_SHIFT)) >> PTE_SHIFT;

  uint64_t *pud = create_pt(pgd, pgd_idx);
  uint64_t *pmd = create_pt(pud, pud_idx);
  uint64_t *pte = create_pt(pmd, pmd_idx);
  void *addr = create_page(pte, pte_idx, size);
  // for (int i = 0; i < 512; i++) {
  //   if (pgd[i]) {
  //     uart_send_string("pgd-");
  //     uart_int(i);
  //     uart_send_string(": ");
  //     uart_hex_64(pgd[i]);
  //     uart_send_string("\r\n");
  //   }
  // }
  // for (int i = 0; i < 512; i++) {
  //   if (pud[i]) {
  //     uart_send_string("pud-");
  //     uart_int(i);
  //     uart_send_string(": ");
  //     uart_hex_64(pud[i]);
  //     uart_send_string("\r\n");
  //   }
  // }
  // for (int i = 0; i < 512; i++) {
  //   if (pmd[i]) {
  //     uart_send_string("pmd-");
  //     uart_int(i);
  //     uart_send_string(": ");
  //     uart_hex_64(pmd[i]);
  //     uart_send_string("\r\n");
  //   }
  // }
  // for (int i = 0; i < 512; i++) {
  //   if (pte[i]) {
  //     uart_send_string("pte-");
  //     uart_int(i);
  //     uart_send_string(": ");
  //     uart_hex_64(pte[i]);
  //     uart_send_string("\r\n");
  //   }
  // }
  return addr;
}