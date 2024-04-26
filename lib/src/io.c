#include "io.h"

#include "string.h"
#include "uart.h"

size_t vsprintk(char *dst, char *fmt, __builtin_va_list args) {
  char *dst_orig = dst;

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;

      // escape %
      if (*fmt == '%') {
        goto puts;
      }

      // string
      else if (*fmt == 's') {
        char *p = __builtin_va_arg(args, char *);
        while (*p) {
          *dst++ = *p++;
        }
      }

      // number
      else if (*fmt == 'd') {
        int arg = __builtin_va_arg(args, int);
        char buf[11];
        char *p = itoa(arg, buf);
        while (*p) {
          *dst++ = *p++;
        }
      }

      /* hex number */
      else if (*fmt == 'x') {
        size_t num = __builtin_va_arg(args, size_t);

        char buf[17];
        char *p = itohex(num, buf);
        while (*p) {
          *dst++ = *p++;
        }
      }
    } else {
    puts:
      *dst++ = *fmt;
    }
    fmt++;
  }
  *dst = '\0';

  // return count of written bytes
  return dst - dst_orig;
}

size_t sprintk(char *dst, char *fmt, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, fmt);

  return vsprintk(dst, fmt, args);
}
