#include "board/mini-uart.hpp"

#include "board/gpio.hpp"
#include "board/mmio.hpp"
#include "board/peripheral.hpp"
#include "nanoprintf.hpp"
#include "ringbuffer.hpp"
#include "util.hpp"

decltype(&mini_uart_getc_raw) mini_uart_getc_raw_fp;
decltype(&mini_uart_getc) mini_uart_getc_fp;
decltype(&mini_uart_putc_raw) mini_uart_putc_raw_fp;
decltype(&mini_uart_putc) mini_uart_putc_fp;

#define RECEIVE_INT  0
#define TRANSMIT_INT 1

RingBuffer rbuf, wbuf;

void set_ier_reg(bool enable, int bit) {
  (enable ? setbit : clearbit)(AUX_MU_IER_REG, bit);
}

void mini_uart_use_async(bool use) {
  if (use) {
    set_aux_irq(true);
    set_ier_reg(true, RECEIVE_INT);
    mini_uart_getc_raw_fp = mini_uart_getc_raw_async;
    mini_uart_getc_fp = mini_uart_getc_async;
    mini_uart_putc_raw_fp = mini_uart_putc_raw_async;
    mini_uart_putc_fp = mini_uart_putc_async;
  } else {
    // TODO: handle rbuf / wbuf
    set_aux_irq(false);
    set_ier_reg(false, RECEIVE_INT);
    set_ier_reg(false, TRANSMIT_INT);
    mini_uart_getc_raw_fp = mini_uart_getc_raw_sync;
    mini_uart_getc_fp = mini_uart_getc_sync;
    mini_uart_putc_raw_fp = mini_uart_putc_raw_sync;
    mini_uart_putc_fp = mini_uart_putc_sync;
  }
}

void mini_uart_handler() {
  auto iir = get32(AUX_MU_IIR_REG);
  if (iir & (1 << 0))
    return;

  if (iir & (1 << 1)) {
    // Transmit holding register empty
    if (wbuf.empty()) {
      set_ier_reg(false, TRANSMIT_INT);
    } else {
      set32(AUX_MU_IO_REG, wbuf.pop());
    }
  } else if (iir & (1 << 2)) {
    // Receiver holds valid byte
    rbuf.push(get32(AUX_MU_IO_REG) & MASK(8));
  }
}

char mini_uart_getc_raw_async() {
  return rbuf.pop();
}

void mini_uart_putc_raw_async(char c) {
  set_ier_reg(true, TRANSMIT_INT);
  wbuf.push(c);
}

char mini_uart_getc_raw_sync() {
  while ((get32(AUX_MU_LSR_REG) & 1) == 0)
    NOP;
  return get32(AUX_MU_IO_REG) & MASK(8);
}

void mini_uart_putc_raw_sync(char c) {
  while ((get32(AUX_MU_LSR_REG) & (1 << 5)) == 0)
    NOP;
  set32(AUX_MU_IO_REG, c);
}

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

  mini_uart_use_async(false);
}

char mini_uart_getc_async() {
  char c = mini_uart_getc_raw_async();
  return c == '\r' ? '\n' : c;
}

char mini_uart_getc_sync() {
  char c = mini_uart_getc_raw_sync();
  return c == '\r' ? '\n' : c;
}

void mini_uart_putc_async(char c) {
  if (c == '\n')
    mini_uart_putc_raw_async('\r');
  mini_uart_putc_raw_async(c);
}

void mini_uart_putc_sync(char c) {
  if (c == '\n')
    mini_uart_putc_raw_sync('\r');
  mini_uart_putc_raw_sync(c);
}

char mini_uart_getc_raw() {
  return mini_uart_getc_raw_fp();
}
char mini_uart_getc() {
  return mini_uart_getc_fp();
}
void mini_uart_putc_raw(char c) {
  return mini_uart_putc_raw_fp(c);
}
void mini_uart_putc(char c) {
  return mini_uart_putc_fp(c);
}

int mini_uart_getline_echo(char* buffer, int length) {
  if (length <= 0)
    return -1;
  int r = 0;
  for (char c; r < length;) {
    c = mini_uart_getc();
    if (c == '\n') {
      mini_uart_putc_raw('\r');
      mini_uart_putc_raw('\n');
      break;
    }
    if (r + 1 == length)
      continue;
    switch (c) {
      case 8:     // ^H
      case 0x7f:  // backspace
        if (r > 0) {
          buffer[r--] = 0;
          mini_uart_puts("\b \b");
        }
        break;
      case 0x15:  // ^U
        while (r > 0) {
          buffer[r--] = 0;
          mini_uart_puts("\b \b");
        }
        break;
      case '\t':  // skip \t
        break;
      default:
        buffer[r++] = c;
        mini_uart_putc_raw(c);
    }
  }
  buffer[r] = '\0';
  return r;
}

void mini_uart_npf_putc(int c, void* /* ctx */) {
  mini_uart_putc(c);
}

int mini_uart_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int size = npf_vpprintf(&mini_uart_npf_putc, NULL, format, args);
  va_end(args);
  return size;
}

void mini_uart_npf_putc_sync(int c, void* /* ctx */) {
  mini_uart_putc_sync(c);
}

int mini_uart_printf_sync(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int size = npf_vpprintf(&mini_uart_npf_putc, NULL, format, args);
  va_end(args);
  return size;
}

void mini_uart_puts(const char* s) {
  for (char c; (c = *s); s++)
    mini_uart_putc(c);
}

void mini_uart_print_hex(string_view view) {
  for (auto c : view)
    mini_uart_printf("%02x", c);
}
void mini_uart_print_str(string_view view) {
  for (auto c : view)
    mini_uart_putc(c);
}

void mini_uart_print(string_view view) {
  bool printable = true;
  for (int i = 0; i < view.size(); i++) {
    auto c = view[i];
    printable &= (0x20 <= c and c <= 0x7e) or (i + 1 == view.size() and c == 0);
  }
  if (printable)
    mini_uart_print_str(view);
  else
    mini_uart_print_hex(view);
}
