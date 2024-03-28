#include "peripherals/aux.h"
#include "peripherals/gpio.h"
#include "utli.h"

void uart_init() {
  /* Initialize UART */
  set(AUX_ENABLES, *AUX_ENABLES | 1);  // Enable mini UART
  set(AUX_MU_CNTL, 0);                 // Disable TX, RX during configuration
  set(AUX_MU_IER, 0);                  // Disable interrupt
  set(AUX_MU_LCR, 3);                  // Set the data size to 8 bit
  set(AUX_MU_MCR, 0);                  // Don't need auto flow control
  set(AUX_MU_BAUD, 270);               // Set baud rate to 115200
  // set(AUX_MU_IIR, 6);                  // No FIFO

  /* Map UART to GPIO Pins */

  // 1. Change GPIO 14, 15 to alternate function
  register unsigned int r = get(GPFSEL1);
  r &= ~((7 << 12) | (7 << 15));  // Reset GPIO 14, 15
  r |= (2 << 12) | (2 << 15);     // Set ALT5
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

  // 3. Enable TX, RX
  set(AUX_MU_CNTL, 3);
}

char uart_read() {
  // Check data ready field
  do {
    asm volatile("nop");
  } while (!(get(AUX_MU_LSR) & 0x01));
  // Read
  char r = (char)get(AUX_MU_IO);
  // Convert carrige return to newline
  return r == '\r' ? '\n' : r;
}

char uart_read_raw() {
  // Check data ready field
  do {
    asm volatile("nop");
  } while (!(get(AUX_MU_LSR) & 0x01));
  // Read
  char r = (char)get(AUX_MU_IO);
  // Convert carrige return to newline
  return r;
}

void uart_write(unsigned int c) {
  // Check transmitter idle field
  do {
    asm volatile("nop");
  } while (!(get(AUX_MU_LSR) & 0x20));
  // Write
  set(AUX_MU_IO, c);
}

void uart_send_string(const char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    uart_write((char)str[i]);
  }
}

void uart_flush() {
  while (*AUX_MU_LSR & 0x01) {
    *AUX_MU_IO;
  }
}

void uart_int(unsigned int d) {
  if (d == 0) {
    uart_write('0');
  }
  unsigned char tmp[10];
  int total = 0;
  while (d > 0) {
    tmp[total] = '0' + (d % 10);
    d /= 10;
    total++;
  }
  int n;
  for (n = total - 1; n >= 0; n--) {
    uart_write(tmp[n]);
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

void uart_hex_64(unsigned long int d) {
  unsigned long int n;
  int c;
  for (c = 60; c >= 0; c -= 4) {
    n = (d >> c) & 0xF;
    n += (n > 9) ? 0x37 : 0x30;  // 0x30 : 0 , 0x37+10 = 0x41 : A
    uart_write(n);
  }
}
