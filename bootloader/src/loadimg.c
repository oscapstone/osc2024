#include "uart1.h"
#include "utli.h"

void loadimg() {
  uart_init();
  uart_flush();
  uart_send_string("\rBootloader...\r\n");
  wait_usec(2000000);
  uart_send_string("Send image via UART now!\r\n");

  int img_size = 0, i;
  for (i = 0; i < 4; i++) {
    img_size <<= 8;
    img_size |= (int)uart_read_raw();
  }
  uart_send_string("Sucessfully get img_size!\r\n");
  uart_send_string("img_size: ");
  uart_int(img_size);
  uart_send_string("\r\n");

  char *kernel = (char *)0x80000;

  for (i = 0; i < img_size; i++) {
    char b = uart_read_raw();
    *(kernel + i) = b;
    // if (i % 1000 == 0)
    //     uart_printf("\rreceived byte#: %d", i);
  }

  uart_send_string("All received\r\n");
  asm volatile(
      "mov x30, 0x80000;"
      "ret;");
}