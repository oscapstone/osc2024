#pragma once

#include "util.hpp"

inline uint8_t log2c(uint64_t x) {
  return x ? sizeof(x) * 8 - __builtin_clzll(x - 1) : 0;
}
