#include "shell.h"

#include "command.h"
#include "mem.h"
#include "str.h"
#include "uart.h"

static void welcome_msg() {
  cmd_info();
  uart_puts(
      "*******************************\n"
      "*** YADOS 0.06 for OSC 2024 ***\n"
      "*******************************\n"
      "Hopefully this will be Yet Another Dope OS!\n");
  uart_putc(NEWLINE);
  cmd_hello();
}

void run_shell() {
  welcome_msg();

  char *buffer = kmalloc(SHELL_BUF_SIZE, 1);
  while (1) {
    uart_putc(NEWLINE);
    uart_puts("# ");
    read_user_input(buffer);
    if (exec_command(buffer)) {
      uart_log(WARN, "Command not found.\n");
    }
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
        uart_log(WARN, "ESC detected. Clearing buffer...\n");
        uart_log(INFO, "Press Spacebar or Enter to continue.\n");
        while (1) {
          char c = uart_getc();
          if (c == ' ' || c == NEWLINE) break;
        }
        buf[0] = '\0';
        uart_clear();
        return;
      default:
        if (c >= 32 && c <= 126) {
          uart_putc(c);
          buf[idx++] = c;
        }
    }
  }
  uart_putc(NEWLINE);
  uart_log(WARN, "Buffer overflow. Please re-enter your command.\n");
  buf[0] = '\0';
}

int exec_command(const char *input) {
  if (strlen(input) == 0) return 0;
  struct command *cmd = cmd_list;
  while (strcmp(cmd->name, END_OF_COMMAND_LIST)) {
    if (!strcmp(cmd->name, input)) {
      cmd->func();
      return 0;
    }
    cmd++;
  }
  return -1;
}