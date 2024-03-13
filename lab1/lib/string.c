#include "string.h"

int strcmp(char *a, char *b) {
  while (*a && (*a == *b)) {
    a++, b++;
  }
  return *(const unsigned char *)a - *(const unsigned char *)b;
}
