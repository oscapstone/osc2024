#include "string.h"

void memzero(void* start, void* end) {
  for (char* i = (char*)start; i != (char*)end; i++)
    *i = 0;
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
  while ((c1 = *s1) && (c2 = *s2) && (d = c1 - c2) != 0)
    s1++, s2++;
  return d;
}

int strncmp(const char* s1, const char* s2, int n) {
  char c1, c2;
  int d;
  for (int i = 0; i < n && (c1 = *s1) && (c2 = *s2) && (d = c1 - c2) != 0; i++)
    s1++, s2++;
  return d;
}
