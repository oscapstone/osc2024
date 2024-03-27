#include "uart.h"

void load_img() {
  unsigned int size = 0;
  unsigned char *size_buffer = (unsigned char *)&size;

  for (int i = 0; i < 8; i++) {
    size_buffer[i] = uart_read();
  }
  uart_printf("Image size: %d\n", size);

  char *kernel = (char *)0x80000;
  while (size--) {
    *(kernel++) = uart_read();
  }

  uart_println("Image loaded");

  /* In AArch64 state, the Link Register(LR) stores the return address when a
   * subroutine call is made */
  asm volatile("mov x30, 0x80000;");
  asm volatile("ret");
}

void main() {
  // set up serial console
  uart_init();

  uart_println("Start Bootloading...");
  load_img();
}
