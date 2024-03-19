#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "initramfs.hpp"

extern "C" void kernel_main() {
  mini_uart_setup();
  mini_uart_puts("Hello Kernel!\n");

  initramfs.init((char*)0x20000000);

  char buf[0x100];
  for (;;) {
    mini_uart_puts("$ ");
    int len = mini_uart_getline_echo(buf, sizeof(buf));
    if (len <= 0)
      continue;
    runcmd(buf, len);
  }
}
