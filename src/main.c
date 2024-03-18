#include "board/mini-uart.h"
#include "cmd.h"
#include "string.h"

void kernel_main() {
  mini_uart_setup();
  mini_uart_puts("Hello World!\n");

  char buf[0x100];
  for (;;) {
    mini_uart_puts("$ ");
    int len = mini_uart_getline_echo(buf, sizeof(buf));
    if (len <= 0)
      continue;
    const cmd_t* cmd = cmds;
    for (; cmd != cmds_end; cmd++)
      if (!strcmp(buf, cmd->name))
        break;
    if (cmd != cmds_end) {
      cmd->fp();
    } else {
      mini_uart_printf("command not found: %s\n", buf);
    }
  }
}
