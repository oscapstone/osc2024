#include "utils.h"

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void int_to_decimal(char *buf, int value) {
  char *p = buf;
  int negative = value < 0;
  if (negative) {
    value = -value;
  }

  do {
    *p++ = (value % 10) + '0';
    value /= 10;
  } while (value > 0);

  if (negative) {
    *p++ = '-';
  }
  *p = '\0';

  char *start = buf, *end = p - 1;
  char temp;
  while (start < end) {
    temp = *start;
    *start = *end;
    *end = temp;
    start++;
    end--;
  }
}

void uint_to_hex(char *buf, unsigned int value) {
  char *p = buf;
  int nibble;

  do {
    nibble = value & 0xF;
    *p++ = (nibble < 10) ? (nibble + '0') : (nibble - 10 + 'A');
    value >>= 4;
  } while (value > 0);
  *p = '\0';

  char *start = buf, *end = p - 1;
  char temp;
  while (start < end) {
    temp = *start;
    *start = *end;
    *end = temp;
    start++;
    end--;
  }
}

void my_vsprintf(char *buf, const char *fmt, __builtin_va_list args) {
  const char *p;
  char temp[20];
  int d;
  unsigned int u;
  char *s, *out = buf;

  for (p = fmt; *p != '\0'; p++) {
    if (*p != '%') {
      *out++ = *p;
      continue;
    }

    switch (*++p) {
    case 'c':
      *out++ = (char)__builtin_va_arg(args, int);
      break;
    case 'd':
      d = __builtin_va_arg(args, int);
      int_to_decimal(temp, d);
      for (s = temp; *s; s++) {
        *out++ = *s;
      }
      break;
    case 'x':
      u = __builtin_va_arg(args, unsigned int);
      uint_to_hex(temp, u);
      for (s = temp; *s; s++) {
        *out++ = *s;
      }
      break;
    case 's':
      s = __builtin_va_arg(args, char *);
      while (*s) {
        *out++ = *s++;
      }
      break;
    default:
      *out++ = *p;
      break;
    }
  }

  *out = '\0';
}
