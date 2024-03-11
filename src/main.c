#include "mini-uart.h"

void memzero(void* start, void* end) {
  for (long* i = start; i != end; i++)
    *i = 0;
}

void kernel_main() {
  mini_uart_setup();

  const char str[] = "Hello World!\n";
  mini_uart_puts(str);

  for (char c; (c = mini_uart_getc());) {
    if (c == '\n')
      mini_uart_putc('\r');
    mini_uart_putc(c);
  }
}
