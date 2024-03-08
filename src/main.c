#include "mini-uart.h"

void memzero(void* start, void* end) {
  for (long* i = start; i != end; i++)
    *i = 0;
}

void kernel_main() {
  mini_uart_setup();

  const char str[] = "Hello World!\r\n";
  for (int i = 0; str[i]; i++)
    mini_uart_putc(str[i]);

  for (;;)
    ;
}
