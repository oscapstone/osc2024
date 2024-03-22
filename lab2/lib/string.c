#include "string.h"

int strcmp(char *a, char *b) {
  while (*a && (*a == *b)) {
    a++, b++;
  }
  return *(const unsigned char *)a - *(const unsigned char *)b;
}

char *strcpy(char *dest, const char *src) {
  char *start = dest;

  while (*src != '\0') {
    *dest = *src;
    dest++;
    src++;
  }

  *dest = '\0';
  return start;
}
