#include "uart0.h"
#include "utli.h"

void loadimg() {
  uart_init();
  uart_flush();
  uart_printf("Bootloader...\n");
  wait_usec(2000000);
  uart_printf("Send image via UART now!\n");

  // big endian
  int img_size = 0, i;
  for (i = 0; i < 4; i++) {
    img_size <<= 8;
    img_size |= (int)uart_read_raw();
  }
  uart_printf("Sucessfully get img_size!\n");
  uart_printf("img_size: %d\n", img_size);

  // big endian
  int img_checksum = 0;
  for (i = 0; i < 4; i++) {
    img_checksum <<= 8;
    img_checksum |= (int)uart_read_raw();
  }
  uart_printf("Sucessfully get img_checksum!\n");
  uart_printf("img_checksum: %d\n", img_checksum);

  char *kernel = (char *)0x80000;

  for (i = 0; i < img_size; i++) {
    char b = uart_read_raw();
    *(kernel + i) = b;
    img_checksum -= (int)b;
    // if (i % 1000 == 0)
    //     uart_printf("\rreceived byte#: %d", i);
  }

  uart_write('\n');
  if (img_checksum != 0) {
    uart_printf("checksum check failed!\n");
    uart_printf("Please reupload..\n");
    asm volatile(
        "mov x30, 0x60000;"
        "ret;");
  } else {
    uart_printf("checksum correct!\n");
    asm volatile(
        "mov x30, 0x80000;"
        "ret;");
  }
}