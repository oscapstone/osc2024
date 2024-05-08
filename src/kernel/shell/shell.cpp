#include "shell/shell.hpp"

#include "board/mini-uart.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"
#include "string.hpp"

void shell(void*) {
  char buf[0x100];
  for (;;) {
    kputs("$ ");
    memzero(buf, buf + sizeof(buf));
    int len = mini_uart_getline_echo(buf, sizeof(buf));
    if (len <= 0)
      continue;
    runcmd(buf, len);
  }
}
