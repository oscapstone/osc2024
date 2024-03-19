#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "cpio.hpp"

extern "C" void kernel_main() {
  mini_uart_setup();
  mini_uart_puts("Hello Kernel!\n");

  CPIO cpio((char*)0x20000000);
  for (auto it : cpio) {
    mini_uart_printf("%c\t%d\t%s\n", "-d"[it->isdir()], it -> filesize(),
                     it -> name_ptr());
  }

  char buf[0x100];
  for (;;) {
    mini_uart_puts("$ ");
    int len = mini_uart_getline_echo(buf, sizeof(buf));
    if (len <= 0)
      continue;
    runcmd(buf, len);
  }
}
