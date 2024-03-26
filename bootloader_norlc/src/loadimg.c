#include "uart0.h"
#include "utli.h"

void loadimg() {
  uart_init();
  uart_flush();
  uart_printf("Bootloader...\n");
  wait_usec(2000000);
  uart_printf("Send image via UART now!\n");

  int img_size = 0, i;
  for (i = 0; i < 4; i++) {
    img_size <<= 8;
    img_size |= (int)uart_read_raw();
  }
  uart_printf("Sucessfully get img_size!\n");
  uart_printf("img_size: %d\n", img_size);

  char *kernel = (char *)0x80000;

  for (i = 0; i < img_size; i++) {
    char b = uart_read_raw();
    *(kernel + i) = b;
    // if (i % 1000 == 0)
    //     uart_printf("\rreceived byte#: %d", i);
  }

  uart_printf("All received\n");
  asm volatile(
      "mov x30, 0x80000;"
      "ret;");
}