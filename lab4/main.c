#include "include/allocator.h"
#include "include/buddy_system.h"
#include "include/dtb.h"
#include "include/exception.h"
#include "include/heap.h"
#include "include/shell.h"
#include "include/timer.h"
#include "include/types.h"
#include "include/uart.h"

extern char *dtb_ptr;
extern irq_task_min_heap_t *irq_task_heap;
extern char _start;
extern char _end;
extern char *heap_ptr;

unsigned int get_stack_pointer() {
  unsigned int sp;
  __asm__ volatile("mov %0, sp" : "=r"(sp));
  return sp;
}

int main(char *arg) {
  dtb_ptr = arg;
  uart_init();
  uart_sendline("Code start: 0x%p.\n", (unsigned long)&_start);
  uart_sendline("Code end: 0x%p.\n", (unsigned long)&_end);
  uart_sendline("Stack pointer (sp): 0x%p.\n",
                (unsigned long)get_stack_pointer());
  uart_sendline("DTB header at address 0x%p.\n", (unsigned long)dtb_ptr);
  dtb_initramfs_init();
  heap_init();
  uart_getc();
  startup_memory_block_table_init();
  buddy_system_init();
  memory_pool_init();
  uart_sendline("heap_ptr = 0x%p.\n", (unsigned long)heap_ptr);

  timer_list_init();
  irq_task_min_heap_init(irq_task_heap, IRQ_HEAP_CAPACITY);
  core_timer_enable();
  uart_interrupts_enable();
  el1_interrupt_enable();

  shell_run();
  return 0;
}