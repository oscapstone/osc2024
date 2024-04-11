#include "uart.h"

#include "gpio.h"
#include "io.h"
#include "utils.h"

/* Auxilary mini UART registers */
#define AUX_ENABLE ((volatile unsigned int *)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR ((volatile unsigned int *)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR ((volatile unsigned int *)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR ((volatile unsigned int *)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR ((volatile unsigned int *)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH ((volatile unsigned int *)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL ((volatile unsigned int *)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT ((volatile unsigned int *)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD ((volatile unsigned int *)(MMIO_BASE + 0x00215068))

void uart_init() {
  // function selector
  register unsigned int selector;

  /* map UART1 to GPIO pins */
  selector = *GPFSEL1;

  // clean gpio14, gpio15
  // SEL0 -> 1-10, SEL1 -> 11-20
  selector &= ~((7 << 12) | (7 << 15));

  // alt5
  selector |= (2 << 12) | (2 << 15);

  *GPFSEL1 = selector;

  // enable pins 14 and 15
  *GPPUD = 0;
  wait_cycles(150);

  *GPPUDCLK0 = (1 << 14) | (1 << 15);
  wait_cycles(150);

  // flush GPIO setup
  *GPPUDCLK0 = 0;

  /* initialize UART */
  // enable mini uart
  *AUX_ENABLE |= 1;

  // disable transmitter and receiver during configuration
  *AUX_MU_CNTL = 0;

  // disable interrupt
  *AUX_MU_IER = 0;

  // set the data size to 8 bit
  *AUX_MU_LCR = 3;

  // do not nedd auto flow control
  *AUX_MU_MCR = 0;

  // set baud rate to 115200
  *AUX_MU_BAUD = 270;

  *AUX_MU_IIR = 6;
  *AUX_MU_CNTL = 3;  // enable Tx, Rx
}

void uart_write(unsigned int c) {
  // wait until we can send
  do {
    asm volatile("nop");
  } while (!(*AUX_MU_LSR & 0x20));
  // write char to the buffer
  *AUX_MU_IO = c;
}

char uart_read_raw() {
  char r;
  // wait until something is in the buffer
  do {
    asm volatile("nop");
  } while (!(*AUX_MU_LSR & 0x01));

  r = (char)(*AUX_MU_IO);

  return r;
}

char uart_read() {
  char r = uart_read_raw();
  // convert carriage return to newline
  return r == '\r' ? '\n' : r;
}

void uart_printf(char *fmt, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, fmt);

  char str_buf[1024];
  vsprintk(str_buf, fmt, args);

  int index = 0;
  while (str_buf[index] && index < 1024) {
    uart_write(str_buf[index++]);
  }
}

void uart_print(char *s) {
  while (*s) {
    if (*s == '\n') {
      // convert newline to carriage return + newline
      uart_write('\r');
    }
    uart_write(*s++);
  }
}

void uart_println(char *s) {
  uart_print(s);

  // newline in the end
  uart_print("\n");
}

void uart_hex(size_t d) {
  unsigned int n;
  int c;

  for (c = 60; c >= 0; c -= 4) {
    // get highest tetrad
    n = (d >> c) & 0xF;

    // 0-9 => '0'-'9', 10-15 => 'A'-'F'
    n += n > 9 ? 0x37 : 0x30;
    uart_write(n);
  }
}
