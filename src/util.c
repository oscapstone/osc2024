#include "util.h"

void memzero(void* start, void* end) {
  for (char* i = start; i != end; i++)
    *i = 0;
}

int strcmp(const char* s1, const char* s2) {
  char c1, c2;
  int d;
  while ((c1 = *s1) && (c2 = *s2) && (d = c1 - c2) != 0)
    s1++, s2++;
  return d;
}
