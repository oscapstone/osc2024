#include "peripherals/uart0.h"

#include "mbox.h"
#include "my_string.h"
#include "peripherals/gpio.h"
#include "peripherals/mbox.h"
#include "utli.h"

void uart_init() {
  set(UART0_CR, 0);  // turn off UART0

  /* Configure UART0 Clock Frequency */
  mbox[0] = 9 * 4;
  mbox[1] = MBOX_CODE_BUF_REQ;

  // tags begin
  mbox[2] = MBOX_TAG_SET_CLOCK_RATE;
  mbox[3] = 12;  // buffer size
  mbox[4] = MBOX_CODE_TAG_REQ;
  mbox[5] = 2;              // UART clock
  mbox[6] = 4000000;        // 4MHz
  mbox[7] = 0;              // clear turbo
  mbox[8] = MBOX_TAG_LAST;  // end tag
  // tags end
  mbox_call(MBOX_CH_PROP);

  /* Map UART to GPIO Pins */
  // 1. Change GPIO 14, 15 to alternate function
  register unsigned int r = get(GPFSEL1);
  r &= ~((7 << 12) | (7 << 15));  // Reset GPIO 14, 15
  r |= (4 << 12) | (4 << 15);     // Set ALT0
  set(GPFSEL1, r);
  // 2. Disable GPIO pull up/down (Because these GPIO pins use alternate
  // functions, not basic input-output) Set control signal to disable
  set(GPPUD, 0);

  // Wait 150 cycles
  wait_cycles(150);

  // Clock the control signal into the GPIO pads
  set(GPPUDCLK0, (1 << 14) | (1 << 15));

  // Wait 150 cycles
  wait_cycles(150);

  // Remove the clock
  set(GPPUDCLK0, 0);

  /* Configure UART0 */
  set(UART0_IBRD, 0x2);        // Set 115200 Baud
  set(UART0_FBRD, 0xB);        // Set 115200 Baud
  set(UART0_LCRH, 0b11 << 5);  // Set word length to 8-bits
  set(UART0_ICR, 0x7FF);       // Clear Interrupts
  set(UART0_CR, 0x301);        /* Enable UART */
}

char uart_read_raw() {
  // Check data ready field
  do {
    asm volatile("nop");
  } while (get(UART0_FR) & 0x10);
  // Read
  return (char)get(UART0_DR);
}

char uart_read() {
  // Check data ready field
  do {
    asm volatile("nop");
  } while (get(UART0_FR) & 0x10);
  // Read
  char r = (char)get(UART0_DR);
  // Convert carrige return to newline
  return r == '\r' ? '\n' : r;
}

void uart_write(unsigned int c) {
  // Check transmitter idle field
  do {
    asm volatile("nop");
  } while (get(UART0_FR) & 0x20);
  // Write
  set(UART0_DR, c);
}

void uart_printf(char *fmt, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  extern volatile unsigned char _end;  // defined in linker

  char *s = (char *)&_end;  // put temporary string after code
  vsprintf(s, fmt, args);

  while (*s) {
    if (*s == '\n') uart_write('\r');
    uart_write(*s++);
  }
}

void uart_flush() {
  while (!(get(UART0_FR) & 0x10)) {
    (void)get(UART0_DR);  // unused variable
  }
}

void uart_hex(unsigned int d) {
  unsigned int n;
  int c;
  for (c = 28; c >= 0; c -= 4) {
    // get highest tetrad
    n = (d >> c) & 0xF;
    // 0-9 => '0'-'9', 10-15 => 'A'-'F'
    n += n > 9 ? 0x37 : 0x30;
    uart_write(n);
  }
}
