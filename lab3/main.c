#include "include/dtb.h"
#include "include/exception.h"
#include "include/shell.h"
#include "include/timer.h"
#include "include/uart.h"

extern char *dtb_ptr;
extern irq_task_min_heap_t *irq_task_heap;

int main(char *arg) {
  dtb_ptr = arg;
  uart_init();
  uart_sendline("DTB header at address 0x%p\n", dtb_ptr);
  dtb_initramfs_init(dtb_ptr);
  heap_init();
  timer_list_init();
  core_timer_enable();
  uart_interrupts_enable();
  irq_task_min_heap_init(irq_task_heap, IRQ_HEAP_CAPACITY);
  el1_interrupt_enable();

  shell_run();

  return 0;
}