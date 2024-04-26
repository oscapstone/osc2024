#include "string.h"

#include "int.h"
#include "uart.h"

char *strtok(char *src, char *dest, size_t n, char delimeter) {
  size_t tok_size = 0;

  while (tok_size < n - 1) {
    if (*src == '\0') {
      break;
    }

    if (*src == delimeter) {
      if (tok_size) break;

      src++;
      continue;
    }

    dest[tok_size++] = *src++;
  }

  dest[tok_size] = '\0';
  return src;
}

int strncmp(const char *str1, const char *str2, size_t size) {
  int index = 0;

  while (index < size) {
    if (str1[index] != str2[index]) {
      return -1;
    }

    // str2[index] == '\0' too
    if (str1[index] == '\0') {
      return 0;
    }

    index++;
  }

  return 0;
}

u32_t strnlen(const char *s, u32_t n) {
  u32_t size = 0;
  while (*s != '\0' && size < n) {
    size++, s++, n++;
  }

  return size;
}

int isspace(char c) {
  char sp[] = {' ', '\t', '\r', '\n', '\v', '\f'};
  for (int i = 0; i < sizeof(sp) / sizeof(char); i++) {
    if (c == sp[i]) return 1;
  }

  return 0;
}

char *itoa(int value, char *s) {
  int idx = 0;

  if (value < 0) {
    value *= -1;
    s[idx++] = '-';
  }

  char tmp[10];
  int tidx = 0;

  // read from least significant digit
  do {
    tmp[tidx++] = '0' + value % 10;
    value /= 10;
  } while (value != 0 && tidx < 11);

  // revserse
  for (int i = tidx - 1; i >= 0; i--) {
    s[idx++] = tmp[i];
  }

  s[idx] = '\0';

  return s;
}

char *itohex(size_t value, char *s) {
  short c, n;
  size_t idx = 0;

  for (c = sizeof(size_t) * 8 - 4; c >= 0; c -= 4) {
    // get highest tetrad
    n = (value >> c) & 0xF;

    // remove prefix 0s
    if (idx == 0 && n == 0) continue;

    // 0-9 => '0'-'9', 10-15 => 'A'-'F'
    n += n > 9 ? 0x37 : 0x30;
    s[idx++] = n;
  }

  if (idx == 0) {
    s[idx++] = '0';
  }

  s[idx] = '\0';

  return s;
}

char tolower(char c) {
  if (c < 'A' || c > 'Z') {
    return c;
  }

  return c + 'a' - 'A';
}

u32_t hex_to_i(char *h, int n) {
  int val = 0;
  while (n--) {
    val <<= 4;
    char c = tolower(*h++);

    if (c >= 'a' && c <= 'f') {
      val += c - 'a' + 10;
      continue;
    }

    val += c - '0';
  }

  return val;
}
