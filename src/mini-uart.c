#include "mini-uart.h"

#include "gpio.h"
#include "util.h"

void mini_uart_setup() {
  // set gpio14
  gpio_fsel_set(GPFSEL1, 12, GPIO_FSEL_ALT5);
  // set gpio15
  gpio_fsel_set(GPFSEL1, 15, GPIO_FSEL_ALT5);

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
  set32(AUX_ENABLES, 0);
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
    ;
  return get32(AUX_MU_IO_REG) & MASK(8);
}

void mini_uart_putc(char c) {
  while ((get32(AUX_MU_LSR_REG) & (1 << 5)) == 0)
    ;
  set32(AUX_MU_IO_REG, c);
}
