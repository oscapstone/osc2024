#include "shell.h"

#include "command.h"
#include "string.h"
#include "uart.h"

static void welcome_msg() {
  cmd_info();
  uart_puts(
      "*******************************\n"
      "*** YADOS 0.04 for OSC 2024 ***\n"
      "*******************************\n"
      "Hopefully this will be Yet Another Dope OS!");
  uart_putc(NEWLINE);
  uart_putc(NEWLINE);
  cmd_hello();
}

void run_shell() {
  welcome_msg();
  while (1) {
    char buffer[SHELL_BUF_SIZE];
    uart_putc(NEWLINE);
    uart_puts("# ");
    read_user_input(buffer);
    exec_command(buffer);
  }
}

void read_user_input(char *buf) {
  int idx = 0;
  while (idx < SHELL_BUF_SIZE) {
    char c = uart_getc();
    switch (c) {
      case NEWLINE:
        uart_putc(NEWLINE);
        buf[idx] = '\0';
        return;
      case DELETE:
        if (idx > 0) {
          idx--;
          uart_putc(BACKSPACE);
          uart_putc(' ');
          uart_putc(BACKSPACE);
        }
        break;
      case ESC:
        uart_putc(NEWLINE);
        uart_puts(
            "ESC detected. Clearing buffer. Press Spacebar or Enter to exit.");
        uart_putc(NEWLINE);
        while (1) {
          char c = uart_getc();
          if (c == ' ' || c == NEWLINE) break;
        }
        buf[0] = '\0';
        return;
      default:
        if (c >= 32 && c <= 126) {
          uart_putc(c);
          buf[idx++] = c;
        }
    }
  }
  uart_putc(NEWLINE);
  uart_puts("Buffer overflow. Please re-enter your command.");
  uart_putc(NEWLINE);
  buf[0] = '\0';
}

int exec_command(const char *input) {
  if (strlen(input) == 0) return 0;
  struct command *cmd = commands;
  while (1) {
    if (!strcmp(cmd->name, END_OF_COMMAND_LIST)) {
      uart_puts("Command not found.");
      uart_putc(NEWLINE);
      break;
    }
    if (!strcmp(cmd->name, input)) {
      cmd->func();
      break;
    }
    cmd++;
  }
  return 0;
}