#include "uart.h"
#include "utils.h"

void uart_init() {
  register unsigned int r;

  /* map UART1 to GPIO pins */
  r = *GPFSEL1;
  r &= ~(7 << 12); // clean gpio14
  r |= 2 << 12;    // set gpio14 to alt5
  r &= ~(7 << 15); // clean gpio15
  r |= 2 << 15;    // set gpio15 to alt5
  *GPFSEL1 = r;

  /* enable pin 14, 15 - ref: Page 101 */
  *GPPUD = 0;
  r = 150;
  while (r--) {
    asm volatile("nop");
  }
  *GPPUDCLK0 = (1 << 14) | (1 << 15);
  r = 150;
  while (r--) {
    asm volatile("nop");
  }
  *GPPUDCLK0 = 0;

  /* initialize UART */
  *AUX_ENABLE |= 1; // enable UART1
  *AUX_MU_CNTL = 0; // disable TX/RX

  /* configure UART */
  *AUX_MU_IER = 0;    // disable interrupt
  *AUX_MU_LCR = 3;    // 8 bit data size
  *AUX_MU_MCR = 0;    // disable flow control
  *AUX_MU_BAUD = 270; // 115200 baud rate
  *AUX_MU_IIR = 0xC6; // disable FIFO and clear FIFO

  *AUX_MU_CNTL = 3; // enable TX/RX
}

char uart_get_binary() {
  while (!(*AUX_MU_LSR & 0x01)) {
  };
  char r = (char)(*AUX_MU_IO);
  return r;
}

char uart_getc() {
  while (!(*AUX_MU_LSR & 0x01)) {
  };
  char r = (char)(*AUX_MU_IO);
  return r == '\r' ? '\n' : r;
}

void uart_putc(unsigned int c) {
  while (!(*AUX_MU_LSR & 0x20)) {
  };
  *AUX_MU_IO = c;
}

void uart_send(const char *fmt, ...) {
  char buf[1024];
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  my_vsprintf(buf, fmt, args);
  __builtin_va_end(args);
  char *s = buf;
  while (*s) {
    if (*s == '\n')
      uart_putc('\r');
    uart_putc(*s++);
  }
}