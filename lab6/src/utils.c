#include "utils.h"

int align4(int n) { return n + (4 - n % 4) % 4; }

int atoi(const char *s) {
  int result = 0;
  int sign = 1;
  int i = 0;

  // Skip leading spaces
  while (s[i] == ' ') {
    i++;
  }

  // Handle positive and negative sign
  if (s[i] == '-') {
    sign = -1;
    i++;
  } else if (s[i] == '+') {
    i++;
  }

  // Convert string to integer
  while (s[i] >= '0' && s[i] <= '9') {
    result = result * 10 + (s[i] - '0');
    i++;
  }

  return sign * result;
}

int hextoi(char *s, int n) {
  int r = 0;
  while (n-- > 0) {
    r = r << 4;
    if (*s >= 'A')
      r += *s++ - 'A' + 10;
    else if (*s >= 0)
      r += *s++ - '0';
  }
  return r;
}

int memcmp(const void *m1, const void *m2, int n) {
  const unsigned char *a = m1, *b = m2;
  while (n-- > 0) {
    if (*a != *b) return *a - *b;
    a++;
    b++;
  }
  return 0;
}

/**
 * @brief Copy `n` bytes from `src` to `dest`.
 */
void *memcpy(void *dest, const void *src, int n) {
  char *d = dest;
  const char *s = src;
  while (n--) *d++ = *s++;
  return dest;
}

/**
 * @brief Set `n` bytes of `s` to `c`.
 */
void *memset(void *s, int c, int n) {
  unsigned char *p = s;
  while (n--) *p++ = (unsigned char)c;
  return s;
}