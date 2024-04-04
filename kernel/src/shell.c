#include "shell.h"

#include "command/fireware.h"
#include "command/other.h"
#include "string.h"
#include "uart.h"

void register_cmd(shell_t *s, cmd_t cmd) { s->cmds[s->n_cmds++] = cmd; }

void register_cmds(shell_t *s) {
  register_cmd(s, (cmd_t){.help = "print Hello World!",
                          .name = "hello",
                          .execute = hello_cmd});

  register_cmd(s, (cmd_t){.help = "show board info",  //
                          .name = "board",
                          .execute = board_cmd});

  register_cmd(s, (cmd_t){.help = "reboot the device",
                          .name = "reboot",
                          .execute = reboot_cmd});
}

void handle_line(const shell_t *s, char *line, int n) {
  if (strncmp(line, "help", 4) == 0) {
    uart_println("help:\t print this help menu");
    for (int i = 0; i < s->n_cmds; i++) {
      uart_printf("%s:\t %s\n", s->cmds[i].name, s->cmds[i].help);
    }
    return;
  }

  for (int i = 0; i < s->n_cmds; i++) {
    u32_t cmd_size = strnlen(s->cmds[i].name, 256);
    if (strncmp(s->cmds[i].name, line, cmd_size) != 0) {
      continue;
    }

    char *args = line + cmd_size;
    while (isspace(*args)) args++;

    s->cmds[i].execute(args, n - (args - line));
    return;
  }

  /* unhandled input */
  uart_printf("command not found: %s\n", line);
}

void shell_loop(const shell_t *s) {
  char cmd[256] = {};
  int cmd_index = 0;
  while (1) {
    // prompt
    if (cmd_index == 0) {
      uart_print("$ ");
    }

    // echo user input
    char c = uart_read();
    uart_print(&c);

    cmd[cmd_index++] = c;

    // consume input until newline
    if (c != '\n') {
      continue;
    }

    cmd[cmd_index] = '\0';
    handle_line(s, cmd, cmd_index);

    cmd_index = 0;
  }
}
