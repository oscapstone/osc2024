#include "lib/uart.h"

typedef void (*kernel_main)(char *dbt);

void load_img() {
  unsigned int size = 0;
  unsigned char *size_buffer = (unsigned char *)&size;

  for (int i = 0; i < 8; i++) {
    size_buffer[i] = uart_read();
  }
  uart_printf("Image size: %d\n", size);

  char *kernel = (char *)0x80000;
  for (int i = 0; i < size; i++) {
    kernel[i] = uart_read_raw();
  }

  uart_println("Image loaded");
}

void main(void *dtb) {
  // set up serial console
  uart_init();

  uart_println("Start Bootloading...");
  load_img();

  kernel_main kernel = (kernel_main)0x80000;
  kernel(dtb);
}
