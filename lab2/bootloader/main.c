#include "bsp/uart.h"
#include <stdlib.h>

int main() {
  uart_init();
  uart_puts("Mini UART Bootloader! Please send the kernel image...\n");

  // Get kernel image size
  char buf[10] = {0};
  for (int i = 0; i < 16; i++) {
    buf[i] = uart_recv();
    if (buf[i] == '\n') {
      buf[i] = '\0';
      break;
    }
  }

  uart_puts("Kernel size: ");
  uart_puts(buf);
  uart_puts(" bytes\n");

  uart_puts("Loading the kernel image...\n");
  unsigned int size = atoi(buf);
  char *kernel = (char *)0x80000;
  uart_puts("Before from: ");
  uart_hex(*kernel);
  while (size--) {
    *kernel++ = uart_recv();
  }
  uart_puts("After to: ");
  uart_hex(*kernel);

  asm volatile(""
               "mov x0, x10;"             // move back dtb_base to x0
               "mov x30, 0x80000; ret;"); // Jump to the new kernel

  return 0;
}
