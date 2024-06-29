#include "boot.h"

int main() {
  init_uart();
  uart_puts("\033[2J\033[H");
  uart_puts(
      "UART Bootloader\n"
      "Waiting for kernel...\n");

  // Get kernel image size
  char buf[16] = {0};
  for (int i = 0; i < 16; i++) {
    buf[i] = uart_recv();
    if (buf[i] == '\n') {
      buf[i] = '\0';
      break;
    }
  }

  // Load kernel image
  uart_puts("Kernel size: ");
  uart_puts(buf);
  uart_puts(" bytes.\n");
  uart_puts("Loading the kernel image...\n");

  unsigned int size = atoi(buf);
  char *kernel = (char *)0x80000;
  while (size--) *kernel++ = uart_recv();

  // Restore registers x0 x1 x2 x3
  asm volatile(
      "mov x0, x10\n"
      "mov x1, x11\n"
      "mov x2, x12\n"
      "mov x3, x13\n"
      "mov x30, 0x80000\n"
      "ret\n"  // Jump to the new kernel
  );

  return 0;
}