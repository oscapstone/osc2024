#include "util.h"

int strlen(const char* s) {
  const char* e = s;
  while (*e)
    e++;
  return e - s;
}

int strcmp(const char* s1, const char* s2) {
  while (*s1 && *s2 && *s1 == *s2)
    s1++, s2++;
  return *s1 - *s2;
}
