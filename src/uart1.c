#include "uart1.h"

#include "interrupt.h"
#include "peripherals/aux.h"
#include "peripherals/gpio.h"
#include "string.h"
#include "task.h"
#include "utli.h"

static uint32_t w_f = 0, w_b = 0;
static uint32_t r_f = 0, r_b = 0;
char async_uart_read_buf[MAX_BUF_SIZE];
char async_uart_write_buf[MAX_BUF_SIZE];

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
  register uint32_t r = *GPFSEL1;
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

void uart_write(char c) {
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

void uart_int(uint64_t d) {
  if (d == 0) {
    uart_write('0');
  }
  uint8_t tmp[10];
  int32_t total = 0;
  while (d > 0) {
    tmp[total] = '0' + (d % 10);
    d /= 10;
    total++;
  }

  for (int32_t n = total - 1; n >= 0; n--) {
    uart_write(tmp[n]);
  }
}

void uart_hex(uint32_t d) {
  uart_send_string("0x");
  uint32_t n;
  for (int32_t c = 28; c >= 0; c -= 4) {
    // get highest tetrad
    n = (d >> c) & 0xF;
    // 0-9 => '0'-'9', 10-15 => 'A'-'F'
    n += n > 9 ? 0x37 : 0x30;
    uart_write(n);
  }
}

void uart_hex_64(uint64_t d) {
  uart_send_string("0x");
  uint64_t n;
  for (int32_t c = 60; c >= 0; c -= 4) {
    n = (d >> c) & 0xF;
    n += (n > 9) ? 0x37 : 0x30;  // 0x30 : 0 , 0x37+10 = 0x41 : A
    uart_write(n);
  }
}

static void enable_uart_tx_interrupt() { *AUX_MU_IER |= 2; }

static void disable_uart_tx_interrupt() { *AUX_MU_IER &= ~(2); }

static void enable_uart_rx_interrupt() { *AUX_MU_IER |= 1; }

static void disable_uart_rx_interrupt() { *AUX_MU_IER &= ~(1); }

void disable_uart_interrupt() {
  disable_uart_rx_interrupt();
  disable_uart_tx_interrupt();
}

void enable_uart_interrupt() {
  enable_uart_rx_interrupt();
  *ENABLE_IRQS_1 |= 1 << 29;  // Enable mini uart interrupt, connect the
                              // GPU IRQ to CORE0's IRQ (bit29: AUX INT)
}

void uart_write_async(char c) {
  while ((w_b + 1) % MAX_BUF_SIZE == w_f) {  // full buffer -> wait
    asm volatile("nop");
  }
  OS_enter_critical();
  async_uart_write_buf[w_b++] = c;
  w_b %= MAX_BUF_SIZE;
  OS_exit_critical();
  enable_uart_tx_interrupt();
}

char uart_read_async() {
  while (r_f == r_b) {
    asm volatile("nop");
  }
  OS_enter_critical();
  char r = async_uart_read_buf[r_f++];
  r_f %= MAX_BUF_SIZE;
  OS_exit_critical();
  enable_uart_rx_interrupt();
  return r == '\r' ? '\n' : r;
}

uint32_t uart_send_string_async(const char *str) {
  uint32_t i = 0;
  while ((w_b + 1) % MAX_BUF_SIZE != w_f &&
         str[i] != '\0') {  // full buffer -> wait
    async_uart_write_buf[w_b++] = str[i++];
    w_b %= MAX_BUF_SIZE;
  }
  enable_uart_tx_interrupt();
  return i;
}

uint32_t uart_read_string_async(char *str) {
  uint32_t i = 0;
  while (r_f != r_b) {
    str[i++] = async_uart_read_buf[r_f++];
    r_f %= MAX_BUF_SIZE;
  }
  str[i] = '\0';
  enable_uart_rx_interrupt();
  return i;
}

static void uart_tx_interrupt_handler() {
  if (w_b == w_f) {  // the buffer is empty
    return;
  }
  *AUX_MU_IO = (uint32_t)async_uart_write_buf[w_f++];
  w_f %= MAX_BUF_SIZE;
  enable_uart_tx_interrupt();
}

static void uart_rx_interrupt_handler() {
  if ((r_b + 1) % MAX_BUF_SIZE == r_f) {
    return;
  }
  async_uart_read_buf[r_b++] = (char)(*AUX_MU_IO);
  r_b %= MAX_BUF_SIZE;
  enable_uart_rx_interrupt();
}

void uart_interrupt_handler() {
  if (*AUX_MU_IIR &
      0x2)  // bit[2:1]=01: Transmit holding register empty (FIFO empty)
  {
    disable_uart_tx_interrupt();
    add_task(uart_tx_interrupt_handler, UART_INT_PRIORITY);
  } else if (*AUX_MU_IIR & 0x4)  // bit[2:1]=10: Receiver holds valid byte
                                 // (FIFO hold at least 1 symbol)
  {
    disable_uart_rx_interrupt();
    add_task(uart_rx_interrupt_handler, UART_INT_PRIORITY);
  }
}