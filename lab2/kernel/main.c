#include "bsp/ramfs.h"
#include "bsp/uart.h"
#include "command/all.h"
#include "console.h"
#include "device_tree.h"

int main() {
  uart_init();
  uart_puts("\nWelcome to YJack0000's shell\n");

  // fdt_traverse(init_ramfs_callback);

  struct Console *console = console_create();
  register_command(console, &hello_command);
  register_command(console, &mailbox_command);
  register_command(console, &reboot_command);
  register_command(console, &ls_command);
  register_command(console, &cat_command);
  register_command(console, &test_malloc_command);

  while (1) {
    run_console(console);
  }

  return 0;
}
