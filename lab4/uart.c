#include "include/uart.h"
#include "include/exception.h"
#include "include/irq.h"
#include "include/utils.h"

extern circular_buffer_t tx_buffer, rx_buffer;

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

void uart_sendline(const char *fmt, ...) {
  char buf[BUFFER_SIZE];
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  vsnprintf(buf, BUFFER_SIZE, fmt, args);
  __builtin_va_end(args);
  char *s = buf;
  while (*s) {
    if (*s == '\n')
      uart_putc('\r');
    uart_putc(*s++);
  }
}

void uart_interrupts_enable() {
  *AUX_MU_IER |= 0x3; // Enable transmit and receive interrupts
  *ENABLE_IRQS_1 |= (1 << 29);
}

void uart_interrupts_disable() { *AUX_MU_IER &= ~0x03; }

void uart_async_sendline(const char *fmt, ...) {
  char buf[BUFFER_SIZE];
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  vsnprintf(buf, BUFFER_SIZE, fmt, args);
  __builtin_va_end(args);
  char *ptr = buf;
  while (*ptr != '\0') {
    while (((tx_buffer.head + 1) % BUFFER_SIZE) == tx_buffer.tail) {
      *AUX_MU_IER |= 0x02;
    }
    el1_interrupt_disable();
    tx_buffer.buffer[tx_buffer.head] = *ptr++;
    tx_buffer.head = (tx_buffer.head + 1) % BUFFER_SIZE;
    el1_interrupt_enable();
  }
  *AUX_MU_IER |= 0x02;
}

char uart_async_getc() {
  while (rx_buffer.head == rx_buffer.tail) {
    *AUX_MU_IER |= 0x01;
  }
  el1_interrupt_disable();
  char r = rx_buffer.buffer[rx_buffer.tail];
  rx_buffer.tail = (rx_buffer.tail + 1) % BUFFER_SIZE;
  el1_interrupt_enable();
  return r == '\r' ? '\n' : r;
}

void uart_tx_handler(char *arg) {
  while (tx_buffer.head != tx_buffer.tail) {
    while (!(*AUX_MU_LSR & 0x20)) {
    }
    *AUX_MU_IO = tx_buffer.buffer[tx_buffer.tail];
    tx_buffer.tail = (tx_buffer.tail + 1) % BUFFER_SIZE;
  }
  if (tx_buffer.head == tx_buffer.tail) {
    *AUX_MU_IER &= ~0x02;
  }
}

void uart_rx_handler(char *arg) {
  while (*AUX_MU_LSR & 0x01) {
    char r = (char)(*AUX_MU_IO);
    int next = (rx_buffer.head + 1) % BUFFER_SIZE;
    if (next == rx_buffer.tail) {
      *AUX_MU_IER &= ~0x01;
      break;
    }
    rx_buffer.buffer[rx_buffer.head] = r;
    rx_buffer.head = next;
  }
  if (*AUX_MU_LSR & 0x01) {
    *AUX_MU_IER |= 0x01;
  }
}
