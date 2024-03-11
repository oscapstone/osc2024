#include "cmd.h"
#include "mini-uart.h"

void memzero(void* start, void* end) {
  for (long* i = start; i != end; i++)
    *i = 0;
}

void kernel_main() {
  mini_uart_setup();

  const char str[] = "Hello World!\n";
  mini_uart_puts(str);

  const int bufsize = 0x100;
  char buf[bufsize];
  for (;;) {
    mini_uart_puts("$ ");
    int len = mini_uart_getline_echo(buf, bufsize);
    if (len <= 0)
      continue;
    const Cmd_t* cmd = cmds;
    for (; cmd != cmds_end; cmd++)
      if (!strcmp(buf, cmd->name))
        break;
    if (cmd != cmds_end) {
      cmd->fp(cmd);
    } else {
      mini_uart_puts("command not found: ");
      mini_uart_puts(buf);
      mini_uart_puts("\n");
    }
  }
}
