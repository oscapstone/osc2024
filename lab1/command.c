#include "mbox.h"
#include "string.h"
#include "uart.h"

void input_buffer_overflow_message(char cmd[]) {
  uart_puts("Follow command: \"");
  uart_puts(cmd);
  uart_puts("\"... is too long to process.\n");

  uart_puts("The maximum length of input is 64.");
}

void command_help() {
  uart_puts("\n");
  uart_puts("\033[32m\tValid Command:\n");
  uart_puts("\thelp:   print this help.\n");
  uart_puts("\thello:  print \"Hello World!\".\n");
  uart_puts("\treboot: reboort the device.\n");
  uart_puts("\tinfo:   info of revision and ARM memory.\n");
  uart_puts("\tclear:  clear the screen\n\033[0m");
  uart_puts("\n");
}

void command_hello() { uart_puts("Hello World!\n"); }

void command_not_found(char *s) {
  uart_puts("Err: command ");
  uart_puts(s);
  uart_puts(" not found, try <help>\n");
}

void command_reboot() {
  uart_puts("Start Rebooting...\n");

  *PM_RSTC = PM_PASSWORD | 0x20;
  *PM_WDOG = PM_PASSWORD | 100;
}
void command_info() {
  get_board_revision();
  get_ARM_memory();
}

void command_clear() { uart_puts("\033[2J\033[H\033[3J"); }
