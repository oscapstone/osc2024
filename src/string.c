#include "math_.h"
#include "uart1.h"
#include "utli.h"

int strcmp(const char *X, const char *Y) {
  while (*X) {
    if (*X != *Y) break;
    X++;
    Y++;
  }
  return *(const uint8_t *)X - *(const uint8_t *)Y;
}

int strncmp(const char *X, const char *Y, uint32_t n) {
  for (int i = 0; i < n; i++) {
    if (!X[i] || !Y[i]) {
      if (!X[i] && !Y[i]) {
        return 0;
      } else {
        return -1;
      }
    } else {
      if (X[i] != Y[i]) {
        return X[i] - Y[i];
      }
    }
  }
  return 0;
}

uint32_t strlen(const char *s) {
  uint32_t i = 0;
  while (s[i]) {
    i++;
  }
  return i;
}

uint32_t atoi(char *str) {
  uint32_t ret = 0;
  for (int i = 0; str[i] != '\0'; ++i) {
    if (str[i] > '9' || str[i] < '0') {
      return ret;
    }
    ret = ret * 10 + str[i] - '0';
  }
  return ret;
}

uint32_t strcpy(char *dst, const char *src) {
  uint32_t i = 0;
  while (src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
  return i;
}

uint32_t strncpy(char *dst, const char *src, uint32_t len) {
  uint32_t i = 0;
  while (src[i] != '\0' && i < len) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
  return i;
}

void memcpy(void *dest, const void *src, uint64_t len) {
  char *d = dest;
  const char *s = src;
  while (len--) {
    *d++ = *s++;
  }
}

void memset(void *dest, uint8_t val, uint64_t len) {
  uint8_t *d = dest;
  while (len--) {
    *d++ = val;
  }
}
