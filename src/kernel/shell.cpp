#include "shell.hpp"

#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "string.hpp"

void shell() {
  char buf[0x100];
  for (;;) {
    mini_uart_puts("$ ");
    memzero(buf, buf + sizeof(buf));
    int len = mini_uart_getline_echo(buf, sizeof(buf));
    if (len <= 0)
      continue;
    runcmd(buf, len);
  }
}
