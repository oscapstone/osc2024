#include "uart1.h"

#include "my_string.h"
#include "peripherals/aux.h"
#include "peripherals/gpio.h"
#include "utli.h"

unsigned int w_f = 0, w_b = 0;
unsigned int r_f = 0, r_b = 0;
char async_uart_read_buf[MAX_BUF_SIZE];
char async_uart_write_buf[MAX_BUF_SIZE];
extern void enable_interrupt();
extern void disable_interrupt();

void uart_init() {
  /* Initialize UART */
  *AUX_ENABLES |= 0x1;  // Enable mini UART
  *AUX_MU_CNTL = 0;     // Disable TX, RX during configuration
  *AUX_MU_IER = 0;      // Disable interrupt
  *AUX_MU_LCR = 3;      // Set the data size to 8 bit
  *AUX_MU_MCR = 0;      // Don't need auto flow control
  *AUX_MU_BAUD = 270;   // Set baud rate to 115200
  *AUX_MU_IIR = 6;      // No FIFO

  /* Map UART to GPIO Pins */

  // 1. Change GPIO 14, 15 to alternate function
  register unsigned int r = *GPFSEL1;
  r &= ~((7 << 12) | (7 << 15));  // Reset GPIO 14, 15
  r |= (2 << 12) | (2 << 15);     // Set ALT5
  *GPFSEL1 = r;

  // 2. Disable GPIO pull up/down (Because these GPIO pins use alternate
  // functions, not basic input-output) Set control signal to disable
  *GPPUD = 0;

  // Wait 150 cycles
  wait_cycles(150);

  // Clock the control signal into the GPIO pads
  *GPPUDCLK0 = (1 << 14) | (1 << 15);

  // Wait 150 cycles
  wait_cycles(150);

  // Remove the clock
  *GPPUDCLK0 = 0;

  // 3. Enable TX, RX
  *AUX_MU_CNTL = 3;
}

char uart_read() {
  // Check data ready field
  do {
    asm volatile("nop");
  } while (
      !(*AUX_MU_LSR & 0x01));  // bit0 : data ready; this bit is set if receive
                               // FIFO hold at least 1 symbol (data occupy)
  // Read
  char r = (char)*AUX_MU_IO;
  // Convert carrige return to newline
  return r == '\r' ? '\n' : r;
}

void uart_write(unsigned int c) {
  // Check transmitter idle field
  do {
    asm volatile("nop");
  } while (
      !(*AUX_MU_LSR & 0x20));  // bit5 : This bit is set if the
                               // transmit FIFO can accept at least one byte
  // Write
  *AUX_MU_IO = c;  // AUX_MU_IO_REG : used to read/write from/to UART FIFOs
}

void uart_send_string(const char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    uart_write(str[i]);
  }
}

void uart_puts(const char *str) {
  uart_send_string(str);
  uart_write('\r');
  uart_write('\n');
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

void enable_uart_interrupt() {
  *AUX_MU_IER = 0x1;         // enable RX interrupt
  *ENABLE_IRQS_1 = 1 << 29;  // Enable mini uart interrupt, connect the GPU
                             // IRQ to CORE0's IRQ
}

void uart_send_string_async(const char *str) {
  disable_interrupt();
  for (int i = 0; str[i] != '\0'; i++) {
    async_uart_write_buf[w_b++] = str[i];
  };
  async_uart_write_buf[w_b] = '\0';

  uart_hex(*AUX_MU_IER);
  uart_send_string("\r\n");
  *AUX_MU_IER |= 0x2;  // enable TX interrupt
  uart_hex(*AUX_MU_IER);
  uart_send_string("\r\n");
  enable_interrupt();
}

unsigned int uart_read_string_async(char *str) {
  *AUX_MU_IER &= 0x2;  // disable RX interrupt while keep TX interrupt
  unsigned int i;
  for (i = 0; async_uart_read_buf[i] != '\0'; i++) {
    str[i] = async_uart_read_buf[i];
  };
  return i + 1;
}