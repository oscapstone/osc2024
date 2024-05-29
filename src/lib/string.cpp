#include "string.hpp"

#include "nanoprintf.hpp"

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

void memset(void* b, int c, int len) {
  for (char* i = (char*)b; len; len--, i++)
    *i = c;
}

int memcmp(const void* s1, const void* s2, int n) {
  auto s1_ = (const unsigned char*)s1, s2_ = (const unsigned char*)s2;
  int d;
  for (int i = 0; i < n; i++)
    if ((d = *s1_++ - *s2_++) != 0)
      return d;
  return 0;
}

int strlen(const char* s) {
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

const char* strchr(const char* s, char c) {
  while (*s and *s != c)
    s++;
  return s;
}

long strtol(const char* s, const char** endptr, int base, int n) {
  int r = 0, x = 1;
  char c;
  unsigned int d;
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
  for (int i = 0; (n == 0 or i < n) and (c = *s++); i++) {
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

bool operator==(string_view a, string_view b) {
  if (a.size() != b.size())
    return false;
  return !memcmp(a.data(), b.data(), a.size());
}

string operator+(const string& a, const string& b) {
  string s(a.size() + b.size() + 1);
  s += a;
  s += b;
  return s;
}

string to_hex_string(uint64_t value) {
  uint32_t size = npf_snprintf(nullptr, 0, "0x%lx", value) + 1;
  string s(size);
  s += npf_snprintf(s.data(), s.cap(), "0x%lx", value);
  return s;
}
