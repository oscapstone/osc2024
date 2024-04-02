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

inline void set32(addr_t address, uint32_t value) {
  asm volatile("str %w[v],[%[a]]" ::[a] "r"(address), [v] "r"(value));
}
inline uint32_t get32(addr_t address) {
  uint32_t value;
  asm volatile("ldr %w[v],[%[a]]" : [v] "=r"(value) : [a] "r"(address));
  return value;
}
inline void wait_cycle(unsigned cycle) {
  while (cycle--)
    NOP;
}

// ref:
// https://github.com/torvalds/linux/blob/v6.8/tools/lib/perf/mmap.c#L313-L317
#define read_sysreg(r)                         \
  ({                                           \
    uint64_t __val;                            \
    asm volatile("mrs %0, " #r : "=r"(__val)); \
    __val;                                     \
  })

extern "C" {
// start.S
[[noreturn]] void prog_hang();
}
