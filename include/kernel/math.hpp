#pragma once

#include <cstdint>

inline uint8_t log2(uint64_t x) {
  return sizeof(x) * 8 - __builtin_clzll(x);
}
