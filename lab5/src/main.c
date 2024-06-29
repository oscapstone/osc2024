#include "devtree.h"
#include "initramfs.h"
#include "irq.h"
#include "mem.h"
#include "scheduler.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

int main() {
  /* Initialization */
  init_uart();
  fdt_traverse(initramfs_callback);
  init_mem();

  uart_log(INFO, "enable_interrupt()\n");
  enable_interrupt();
  uart_log(INFO, "init_timer()\n");
  init_timer();
  uart_log(INFO, "sched_init()\n");
  sched_init();

  run_shell();

  return 0;
}