#include "io.hpp"

#include "board/mini-uart.hpp"
#include "ds/timeval.hpp"
#include "int/timer.hpp"
#include "nanoprintf.hpp"

char kgetc() {
  auto c = mini_uart_getc_raw();
  return c == '\r' ? '\n' : c;
}

char kgetc_sync() {
  auto c = mini_uart_getc_raw_sync();
  return c == '\r' ? '\n' : c;
}

void kputc(char c) {
  if (c == '\n')
    mini_uart_putc_raw('\r');
  mini_uart_putc_raw(c);
}

void kputc_sync(char c) {
  if (c == '\n')
    mini_uart_putc_raw_sync('\r');
  mini_uart_putc_raw_sync(c);
}

void k_npf_putc(int c, void* /* ctx */) {
  kputc(c);
}

int kvprintf(const char* format, va_list args) {
  return npf_vpprintf(&k_npf_putc, NULL, format, args);
}

int kprintf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int size = kvprintf(format, args);
  va_end(args);
  return size;
}

void k_npf_putc_sync(int c, void* /* ctx */) {
  kputc_sync(c);
}

int kvprintf_sync(const char* format, va_list args) {
  return npf_vpprintf(&k_npf_putc_sync, NULL, format, args);
}

int kprintf_sync(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int size = kvprintf_sync(format, args);
  va_end(args);
  return size;
}

int klog(const char* format, ...) {
  va_list args;
  va_start(args, format);
  kprintf("[" PRTval "] ", FTval(get_current_time()));
  int size = kvprintf(format, args);
  va_end(args);
  return size;
}

void kputs(const char* s) {
  for (char c; (c = *s); s++)
    kputc(c);
}

void kputs_sync(const char* s) {
  for (char c; (c = *s); s++)
    kputc_sync(c);
}

void kprint_hex(string_view view) {
  for (auto c : view)
    kprintf("%02x", c);
}
void kprint_str(string_view view) {
  for (auto c : view)
    kputc(c);
}

void kprint(string_view view) {
  if (view.printable())
    kprint_str(view);
  else
    kprint_hex(view);
}

unsigned kread(char buf[], unsigned size) {
  for (unsigned i = 0; i < size; i++)
    buf[i] = kgetc();
  return size;
}

unsigned kwrite(const char buf[], unsigned size) {
  for (unsigned i = 0; i < size; i++)
    kputc(buf[i]);
  return size;
}
