#include "bsp/uart.h"
#include "kernel/command/all.h"
#include "kernel/console.h"

int main() {
  uart_init();
  uart_puts("\nWelcome to YJack0000's shell\n");
  struct Console *console = console_create();
  register_command(console, &hello_command);
  register_command(console, &mailbox_command);
  register_command(console, &reboot_command);

  while (1) {
    run_console(console);
  }

  return 0;
}
