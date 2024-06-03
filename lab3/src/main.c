#include "devtree.h"
#include "initramfs.h"
#include "irq.h"
#include "malloc.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

int main() {
  /* Initialization */
  uart_init();
  malloc_init();
  enable_interrupt();
  timer_enable_interrupt();
  fdt_traverse(initramfs_callback);

  /* Shell */
  welcome_msg();
  run_shell();

  return 0;
}