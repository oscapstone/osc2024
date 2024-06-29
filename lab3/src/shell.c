#include "shell.h"

#include "command.h"
#include "string.h"
#include "uart.h"

void welcome_msg() {
  cmd_info();
  uart_puts(
      "*******************************\n"
      "*** YADOS 0.03 for OSC 2024 ***\n"
      "*******************************\n"
      "Hopefully this will be Yet Another Dope OS!");
  uart_putc(NEWLINE);
  uart_putc(NEWLINE);
  cmd_hello();
}

void run_shell() {
  while (1) {
    char buffer[SHELL_BUF_SIZE];

    uart_puts("# ");
    read_user_input(buffer);
    exec_command(buffer);

    // Stop the shell if rebooting
    // if (!strcmp(buffer, "reboot")) break;
  }
}

void read_user_input(char *buf) {
  int idx = 0;
  while (idx < SHELL_BUF_SIZE) {
    char c = uart_getc();
    if (c == NEWLINE) {
      uart_putc(c);
      buf[idx] = '\0';
      return;
    }
    if (c >= 32 && c <= 126) {
      uart_putc(c);
      buf[idx++] = c;
    } else if (c == BACKSPACE || c == DELETE) {
      // Handle backspaces
      if (idx > 0) {
        idx--;
        // buf[idx--] = '\0';
        uart_putc(BACKSPACE);
        uart_putc(' ');
        uart_putc(BACKSPACE);
      }
    } else {
      // Ignore unprintable chars
      continue;
    }
  }
  buffer_overflow_message();
}

void buffer_overflow_message() {
  uart_putc(NEWLINE);
  uart_puts("Buffer overflow:");
  uart_putc(NEWLINE);
  uart_puts("Please re-enter your command.");
  uart_putc(NEWLINE);
}

int exec_command(const char *command) {
  int i = 0;

  while (1) {
    if (strlen(command) == 0) {
      return 0;
    }
    if (!strcmp(commands[i].name, END_OF_COMMAND_LIST)) {
      uart_puts("Command not found.");
      uart_putc(NEWLINE);
      return -1;
    }
    if (!strcmp(commands[i].name, command)) {
      commands[i].func();
      return 0;
    }
    i++;
  }
  return 0;
}