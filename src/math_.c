#include "math_.h"

#include "types.h"

int32_t pow(int32_t x, int32_t y) {
  if (y == 0) {
    return 1;
  }
  return x * pow(x, y - 1);
}

uint32_t log2(uint32_t x) {
  uint32_t r = 0, shift;
  shift = (x > 0xFFFF) << 4;
  x >>= shift;
  r |= shift;
  shift = (x > 0xFF) << 3;
  x >>= shift;
  r |= shift;
  shift = (x > 0xF) << 2;
  x >>= shift;
  r |= shift;
  shift = (x > 0x3) << 1;
  x >>= shift;
  r |= shift;
  return r | (x >> 1);
}