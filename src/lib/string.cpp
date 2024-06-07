#include "string.hpp"

void memzero(void* start, void* end) {
  for (char* i = (char*)start; i != (char*)end; i++)
    *i = 0;
}

void* memcpy(void* dst, const void* src, size_t n) {
  auto d = (char*)dst;
  auto s = (const char*)src;
  for (size_t i = 0; i < n; i++)
    *d++ = *s++;
  return dst;
}

void memset(void* b, int c, size_t len) {
  for (char* i = (char*)b; len; len--, i++)
    *i = c;
}

int memcmp(const void* s1, const void* s2, size_t n) {
  auto s1_ = (const unsigned char*)s1, s2_ = (const unsigned char*)s2;
  int d;
  for (size_t i = 0; i < n; i++)
    if ((d = *s1_++ - *s2_++) != 0)
      return d;
  return 0;
}

size_t strlen(const char* s) {
  const char* e = s;
  while (*e)
    e++;
  return e - s;
}

char* strcpy(char* dst, const char* src) {
  while ((*dst = *src))
    dst++, src++;
  return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
  for (size_t i = 0; i < n and (*dst = *src); i++)
    dst++, src++;
  return dst;
}

int strcmp(const char* s1, const char* s2) {
  char c1, c2;
  int d;
  while ((d = (c1 = *s1) - (c2 = *s2)) == 0 && c1 && c2)
    s1++, s2++;
  return d;
}

int strncmp(const char* s1, const char* s2, size_t n) {
  char c1, c2;
  size_t i = 0;
  int d;
  for (; i < n && (d = (c1 = *s1) - (c2 = *s2)) == 0 && c1 && c2; i++)
    s1++, s2++;
  return i == n ? 0 : d;
}

const char* strchr(const char* s, char c) {
  while (*s and *s != c)
    s++;
  return *s ? s : NULL;
}

const char* strchr_or_e(const char* s, char c) {
  while (*s and *s != c)
    s++;
  return s;
}

long strtol(const char* s, const char** endptr, size_t base, size_t n) {
  size_t r = 0, x = 1;
  char c;
  long d;
  if (*s == '-')
    x = -1, s++;
  else if (*s == '+')
    s++;
  if (base == 0) {
    if (*s != '0')
      base = 10;
    else if (*(s + 1) != 'x')
      base = 8, s += 1;
    else
      base = 16, s += 2;
  }
  for (size_t i = 0; (n == 0 or i < n) and (c = *s++); i++) {
    if ('0' <= c and c <= '9')
      d = c - '0';
    else if ('a' <= c and c <= 'z')
      d = c - 'a' + 10;
    else if ('A' <= c and c <= 'Z')
      d = c - 'A' + 10;
    else
      break;
    r = r * base + d;
  }
  if (endptr)
    *endptr = s;
  return r * x;
}

bool isprint(char c) {
  return 0x20 <= c and c <= 0x7e;
}
