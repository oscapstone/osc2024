#include "io.hpp"

#include "board/mini-uart.hpp"
#include "ds/timeval.hpp"
#include "int/timer.hpp"
#include "nanoprintf.hpp"
#include "syscall.hpp"

SYSCALL_DEFINE2(uartread, char*, buf, unsigned, size) {
  return kread(buf, size);
}

SYSCALL_DEFINE2(uartwrite, const char*, buf, unsigned, size) {
  return kwrite(buf, size);
}

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

void klog(const char* format, ...) {
  va_list args;
  va_start(args, format);
  kprintf("[" PRTval "] ", FTval(get_current_time()));
  kvprintf(format, args);
  va_end(args);
}

void klog(string_view view) {
  kprintf("[" PRTval "] ", FTval(get_current_time()));
  kprint(view);
}

void kflush() {
  mini_uart_async_flush();
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

void kprint_str_or_hex(string_view view) {
  if (view.printable())
    kprint_str(view);
  else
    kprint_hex(view);
}

void kprint(string_view view) {
  kwrite(view.data(), view.size());
}

int kgetline_echo(char* buffer, int size) {
  if (size <= 0)
    return -1;

  int r = 0;

  while (true) {
    auto c = kgetc();
    if (c == (char)-1)
      return false;
    if (c == '\r' or c == '\n') {
      kputs("\r\n");
      buffer[r] = '\0';
      break;
    } else {
      switch (c) {
        case 8:     // ^H
        case 0x7f:  // backspace
          if (r > 0) {
            buffer[r--] = 0;
            kputs("\b \b");
          }
          break;
        case 0x15:  // ^U
          while (r > 0) {
            buffer[r--] = 0;
            kputs("\b \b");
          }
          break;
        case '\t':  // skip \t
          break;
        default:
          if (r + 1 < size) {
            buffer[r++] = c;
            kputc(c);
          }
      }
    }
  }

  return r;
}

unsigned kread(void* buf_p, unsigned size) {
  auto buf = (char*)buf_p;
  for (unsigned i = 0; i < size; i++)
    buf[i] = kgetc();
  return size;
}

unsigned kwrite(const void* buf_p, unsigned size) {
  auto buf = (const char*)buf_p;
  for (unsigned i = 0; i < size; i++)
    kputc(buf[i]);
  return size;
}

void khexdump(const void* buf, unsigned size, const char* msg) {
  auto s = (const char*)buf;
  kprintf("%s: %d bytes dumped\n", msg ? msg : __func__, size);
  for (unsigned i = 0; i < size; i += 16) {
    kprintf("0x%04x:  ", i);
    for (unsigned j = 0; j < 16; j++)
      kprintf(i + j < size ? " %02x" : "   ", s[i + j]);
    kprintf("    ");
    for (unsigned j = 0; j < 16; j++) {
      kprintf(i + j < size ? "%c" : " ", isprint(s[i + j]) ? s[i + j] : '.');
    }
    kprintf("\n");
  }
  return;
}
