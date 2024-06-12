#include "devtree.h"
#include "initramfs.h"
#include "irq.h"
#include "mem.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

int main() {
  /* Initialization */
  fdt_traverse(initramfs_callback);
  init_mem();

  init_uart();
  enable_interrupt();
  enable_timer_interrupt();

  run_shell();

  return 0;
}