#include "uart.h"

#include <stdio.h>

#include "irq.h"
#include "mem.h"

static int uart_read_idx = 0;
static int uart_write_idx = 0;
static char uart_read_buffer[UART_BUF_SIZE];
static char uart_write_buffer[UART_BUF_SIZE];

void init_uart() {
  // Configure GPIO pins
  register unsigned int r = *GPFSEL1;
  r &= ~((7 << 12) | (7 << 15));
  r |= (2 << 12) | (2 << 15);
  *GPFSEL1 = r;

  // Disable GPIO pull up/down
  *GPPUD = 0;
  for (int i = 0; i < 150; i++);
  *GPPUDCLK0 = (1 << 14) | (1 << 15);
  for (int i = 0; i < 150; i++);
  *GPPUD = 0;
  *GPPUDCLK0 = 0;

  // Initialize mini UART
  *AUX_ENABLE |= 1;    // Enable mini UART
  *AUX_MU_CNTL = 0;    // Disable Tx and Rx during setup
  *AUX_MU_IER = 0;     // Disable interrupt
  *AUX_MU_LCR = 3;     // Set data size to 8 bits
  *AUX_MU_MCR = 0;     // Disable auto flow control
  *AUX_MU_BAUD = 270;  // Set baud rate to 115200
  *AUX_MU_IIR = 6;     // No FIFO
  *AUX_MU_CNTL = 3;    // Enable Tx and Rx

  // Enable AUX interrupts
  *ENABLE_IRQS_1 |= 1 << 29;
}

char uart_getc() {
  // Check the data ready field on bit 0 of AUX_MU_LSR_REG
  while (!(*AUX_MU_LSR & 0x01));
  char c = (char)(*AUX_MU_IO);  // Read from AUX_MU_IO_REG
  return c == CARRIAGE_RETURN ? NEWLINE : c;
}

void uart_putc(char c) {
  // Add CARRIAGE_RETURN before NEWLINE
  if (c == NEWLINE) uart_putc(CARRIAGE_RETURN);

  // Check the transmitter empty field on bit 5 of AUX_MU_LSR_REG
  while (!(*AUX_MU_LSR & 0x20));
  *AUX_MU_IO = c;  // Write to AUX_MU_IO_REG
}

void uart_puts(const char *s) {
  while (*s) {
    uart_putc(*s++);
  }
}

void uart_clear() { uart_puts("\033[2J\033[H"); }

void uart_hex(unsigned long h) {
  uart_puts("0x");
  unsigned long n;
  int length = h > 0xFFFFFFFF ? 60 : 28;
  for (int c = length; c >= 0; c -= 4) {
    n = (h >> c) & 0xF;
    n += n > 9 ? 0x37 : '0';
    uart_putc(n);
    if (c / 4 % 8 == 0 && c > 0) uart_putc(' ');
  }
}

void uart_addr_range(uintptr_t start, uintptr_t end) {
  uart_hex(start);
  uart_puts("-");
  uart_hex(end - 1);
}

void uart_simple_hex(unsigned int h) {
  uart_puts("0x");
  if (h == 0) {
    uart_putc('0');
    return;
  }
  char buf[10];
  int i = 0;
  unsigned int n;
  while (h > 0) {
    n = h % 16;
    buf[i++] = n + (n > 9 ? 0x37 : '0');
    h /= 16;
  }
  for (int j = i - 1; j >= 0; j--) {
    uart_putc(buf[j]);
  }
}

void uart_dec(unsigned int d) {
  if (d == 0) {
    uart_putc('0');
    return;
  }
  char buf[10];
  int i = 0;
  while (d > 0) {
    buf[i++] = d % 10 + '0';
    d /= 10;
  }
  for (int j = i - 1; j >= 0; j--) {
    uart_putc(buf[j]);
  }
}

void uart_log(int type, const char *msg) {
  switch (type) {
    case INFO:
      uart_puts("[INFO] ");
      break;
    case TEST:
      uart_puts("[TEST] ");
      break;
    case BUDD:
      uart_puts("[BUDD] ");
      break;
    case CACH:
      uart_puts("[CACH] ");
      break;
    case WARN:
      uart_puts("[WARN] ");
      break;
    case ERR:
      uart_puts("[ERR!] ");
      break;
    default:
      uart_puts("[LOG?] ");
      break;
  }
  uart_puts(msg);
}

void enable_uart_tx_interrupt() { *AUX_MU_IER |= 0x02; }

void disable_uart_tx_interrupt() { *AUX_MU_IER &= 0x01; }

void enable_uart_rx_interrupt() { *AUX_MU_IER |= 0x01; }

void disable_uart_rx_interrupt() { *AUX_MU_IER &= 0x2; }

void uart_tx_irq_handler() {
  disable_uart_tx_interrupt();
  // Buffer is not empty -> send characters
  if (uart_write_idx < UART_BUF_SIZE &&
      uart_write_buffer[uart_write_idx] != 0) {
    while (!(*AUX_MU_LSR & 0x20));
    *AUX_MU_IO = uart_write_buffer[uart_write_idx++];
    enable_uart_tx_interrupt();
  }
}

void uart_rx_irq_handler() {
  while (!(*AUX_MU_LSR & 0x01));
  char c = (char)(*AUX_MU_IO);
  uart_read_buffer[uart_read_idx++] = (c == CARRIAGE_RETURN) ? NEWLINE : c;
  if (uart_read_idx >= UART_BUF_SIZE) uart_read_idx = 0;
  enable_uart_rx_interrupt();
}

void uart_async_read(char *buf, int len) {
  disable_uart_rx_interrupt();
  for (int i = 0; i < uart_read_idx && i < len; i++)
    buf[i] = uart_read_buffer[i];
  buf[uart_read_idx] = 0;
  uart_read_idx = 0;
}

void uart_async_write(const char *s) {
  // Copy string to the write buffer
  int len = 0;
  while (*s != '\0') {
    if (len >= UART_BUF_SIZE) {
      uart_log(ERR, "Exceed the UART buffer size\n");
      return;
    }
    if (*s == NEWLINE) {
      // Insert CARRIAGE_RETURN before NEWLINE
      uart_write_buffer[len++] = CARRIAGE_RETURN;
      uart_write_buffer[len++] = NEWLINE;
    } else
      uart_write_buffer[len++] = *s;
    s++;
  }
  uart_write_buffer[len] = '\0';
  uart_write_idx = 0;  // Reset the buffer index
  enable_uart_tx_interrupt();
}