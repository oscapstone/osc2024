#include "int.h"

int align(int num, int n) {
  int mask = ~(n - 1);
  return (num + n - 1) & mask;
}

u32_t be2le_32(u32_t be) {
  char *be_ptr = (char *)&be;
  u32_t le = 0;

  for (size_t i = 0; i < 4; i++) {
    le |= ((u32_t)be_ptr[i] << 8 * (3 - i));
  }

  return le;
}
