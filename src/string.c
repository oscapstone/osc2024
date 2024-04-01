#include "math.h"
#include "uart1.h"
#include "utli.h"

int strcmp(const char *X, const char *Y) {
  while (*X) {
    if (*X != *Y) break;
    X++;
    Y++;
  }
  return *(const unsigned char *)X - *(const unsigned char *)Y;
}

int strncmp(const char *X, const char *Y, unsigned int n) {
  for (int i = 0; i < n; i++) {
    if (!X || !Y) {
      if (!X && !Y) {
        return 0;
      } else {
        return -1;
      }
    } else {
      if (*X != *Y) {
        return *(const unsigned char *)X - *(const unsigned char *)Y;
      }
    }
  }
  return 0;
}

unsigned int strlen(const char *s) {
  unsigned int i = 0;
  while (s[i]) {
    i++;
  }
  return i;
}

unsigned int atoi(char *str) {
  unsigned int ret = 0;
  for (int i = 0; str[i] != '\0'; ++i) {
    if (str[i] > '9' || str[i] < '0') {
      return ret;
    }
    ret = ret * 10 + str[i] - '0';
  }
  return ret;
}

char *memcpy(void *dest, const void *src, unsigned long long len) {
  char *d = dest;
  const char *s = src;
  while (len--) {
    *d++ = *s++;
  }
  return dest;
}