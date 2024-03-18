#include "board/mini-uart.h"

#include "board/gpio.h"
#include "board/mmio.h"
#include "nanoprintf.h"
#include "util.h"

void mini_uart_setup() {
  unsigned int r = get32(GPFSEL1);
  // set gpio14
  r = (r & ~(7 << 12)) | (GPIO_FSEL_ALT5 << 12);
  // set gpio15
  r = (r & ~(7 << 15)) | (GPIO_FSEL_ALT5 << 15);
  set32(GPFSEL1, r);

  // disable pull-up / down
  set32(GPPUD, 0b00);
  // wait 150 cycle
  wait_cycle(150);
  // set cycle for gpio14 & gpio15
  set32(GPPUDCLK0, get32(GPPUDCLK0) | (1 << 14) | (1 << 15));
  // wait 150 cycle
  wait_cycle(150);
  // remove the control signal
  set32(GPPUD, 0);
  // remove clock
  set32(GPPUDCLK0, 0);

  // enable mini uart
  set32(AUX_ENABLES, get32(AUX_ENABLES) | 1);
  // disable tx & rx
  set32(AUX_MU_CNTL_REG, 0);
  // disable interrupt
  set32(AUX_MU_IER_REG, 0);
  // set data size to 8 bit
  set32(AUX_MU_LCR_REG, 3);
  // no auto flow control
  set32(AUX_MU_MCR_REG, 0);
  // baud rate 115200
  set32(AUX_MU_BAUD_REG, 270);
  // no fifo
  set32(AUX_MU_IIR_REG, 6);
  // enable tx & rx
  set32(AUX_MU_CNTL_REG, 3);
}

char mini_uart_getc() {
  while ((get32(AUX_MU_LSR_REG) & 1) == 0)
    NOP;
  char c = get32(AUX_MU_IO_REG) & MASK(8);
  return c == '\r' ? '\n' : c;
}

void mini_uart_putc(char c) {
  while ((get32(AUX_MU_LSR_REG) & (1 << 5)) == 0)
    NOP;
  set32(AUX_MU_IO_REG, c);
}

void mini_uart_puts(const char* s) {
  for (char c; (c = *s); s++) {
    if (c == '\n')
      mini_uart_putc('\r');
    mini_uart_putc(c);
  }
}

int mini_uart_getline_echo(char* buffer, int length) {
  if (length <= 0)
    return -1;
  int r = 0;
  for (char c; r < length;) {
    c = mini_uart_getc();
    if (c == '\n') {
      mini_uart_putc('\r');
      mini_uart_putc('\n');
      break;
    }
    if (c == 0x08 || c == 0x7F || r + 1 == length)
      continue;
    buffer[r++] = c;
    mini_uart_putc(c);
  }
  buffer[r] = '\0';
  return r;
}

void mini_uart_npf_putc(int c, void* ctx) {
  if (c == '\n')
    mini_uart_putc('\r');
  mini_uart_putc(c);
}

int mini_uart_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int size = npf_vpprintf(&mini_uart_npf_putc, NULL, format, args);
  va_end(args);
  return size;
}
