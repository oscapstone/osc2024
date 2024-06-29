#include "virtm.h"

#include "mem.h"
#include "start.h"
#include "uart.h"
#include "utils.h"

extern unsigned long TOTAL_MEMORY;

static void walk(uintptr_t pt, uintptr_t va, uintptr_t pa) {
  uintptr_t *table = (uintptr_t *)pt;
  for (int level = 0; level <= 3; level++) {
    uintptr_t offset = (va >> (39 - 9 * level)) & 0x1FF;
    if (level == 3) {
      table[offset] = pa | PTE_NORMAL_ATTR;
      return;
    }
    if (!table[offset]) {
      uintptr_t *t = kmalloc(PAGE_SIZE, 0);
      uart_log(INFO, "Acquired page table at ");
      uart_hex((uintptr_t)t);
      uart_puts(" for level ");
      uart_dec(level);
      uart_putc(NEWLINE);
      memset(t, 0, PAGE_SIZE);
      table[offset] = TO_PHYS((uintptr_t)t) | PD_TABLE;
    }
    table = (uintptr_t *)TO_VIRT((table[offset] & ~0xFFF));
  }
}

void map_pages(uintptr_t pgd, uintptr_t va, uintptr_t size, uintptr_t pa) {
  for (int i = 0; i < size; i += PAGE_SIZE) walk(pgd, va + i, pa + i);
}

void mapping_user_thread(thread_struct *thread, int gpu_mem_size) {
  // user code
  map_pages((uintptr_t)thread->pgd, 0x0, thread->size,
            (uintptr_t)TO_PHYS(thread->start));
  // stack region
  map_pages((uintptr_t)thread->pgd, USER_STACK, STACK_SIZE,
            (uintptr_t)TO_PHYS(thread->user_stack));
  // GPU
  if (gpu_mem_size > 0)
    map_pages((uintptr_t)thread->pgd, 0x3C000000, gpu_mem_size, 0x3C000000);
}