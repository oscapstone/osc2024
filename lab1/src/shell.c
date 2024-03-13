#include "shell.h"
#include "string.h"
#include "uart.h"

void handle_line(char *line) {
  if (strncmp(line, "help", 4) == 0) {
    uart_println("help\t: print this help menu");
    uart_println("hello\t: print Hello World!");
    uart_println("reboot\t: reboot the device");
    return;
  }

  if (strncmp(line, "hello", 5) == 0) {
    uart_println("Hello World!");
    return;
  }
}

void shell_loop() {
  char cmd[256] = {};
  int cmd_index = 0;
  while (1) {

    // prompt
    if (cmd_index == 0) {
      uart_print("$ ");
    }

    // echo user input
    char c = uart_read();
    uart_write(c);

    cmd[cmd_index++] = c;

    // consume input until newline
    if (c != '\n') {
      continue;
    }

    cmd[cmd_index] = '\0';
    cmd_index = 0;

    handle_line(cmd);
  }
}
