#include "string.hpp"

void memzero(void* start, void* end) {
  for (char* i = (char*)start; i != (char*)end; i++)
    *i = 0;
}

void* memcpy(void* dst, const void* src, int n) {
  auto d = (char*)dst;
  auto s = (const char*)src;
  for (int i = 0; i < n; i++)
    *d++ = *s++;
  return dst;
}

int strlen(const char* s) {
  const char* e = s;
  while (*e)
    e++;
  return e - s;
}

int strcmp(const char* s1, const char* s2) {
  char c1, c2;
  int d;
  while ((d = (c1 = *s1) - (c2 = *s2)) == 0 && c1 && c2)
    s1++, s2++;
  return d;
}

int strncmp(const char* s1, const char* s2, int n) {
  char c1, c2;
  int i = 0, d;
  for (; i < n && (d = (c1 = *s1) - (c2 = *s2)) == 0 && c1 && c2; i++)
    s1++, s2++;
  return i == n ? 0 : d;
}
