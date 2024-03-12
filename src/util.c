#include "util.h"

void memzero(void* start, void* end) {
  for (char* i = start; i != end; i++)
    *i = 0;
}

int strcmp(const char* s1, const char* s2) {
  while (*s1 && *s2 && *s1 == *s2)
    s1++, s2++;
  return *s1 - *s2;
}
