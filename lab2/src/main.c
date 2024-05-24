#include "alloc.h"
#include "devicetree.h"
#include "initramfs.h"
#include "shell.h"
#include "uart.h"

int main() {
  uart_init();
  alloc_init();
  fdt_traverse(initramfs_callback);
  welcome_msg();
  run_shell();
  return 0;
}