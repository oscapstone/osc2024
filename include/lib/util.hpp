#pragma once

#define MASK(bits) ((1 << bits) - 1)
#define NOP        asm volatile("nop")

// ref: https://github.com/v8/v8/blob/12.5.71/src/base/compiler-specific.h#L26
#if defined(__GNUC__)
#define PRINTF_FORMAT(format_param, dots_param) \
  __attribute__((format(printf, format_param, dots_param)))
#else
#define PRINTF_FORMAT(format_param, dots_param)
#endif

#include <cstdint>
using addr_t = volatile char*;

template <uint64_t sz, typename T>
inline T align(T p) {
  return (T)(((uint64_t)p + sz - 1) & ~(sz - 1));
}

extern "C" {
// util-asm.S
void set32(addr_t address, uint32_t value);
uint32_t get32(addr_t address);
void wait_cycle(unsigned cycle);

// start.S
[[noreturn]] void prog_hang();
}
