#pragma once

#define MASK(bits) ((1ll << bits) - 1)
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

template <uint64_t sz, bool up = true, typename T>
inline T align(T p) {
  return (T)(((uint64_t)p + (up ? sz - 1 : 0)) & ~(sz - 1));
}
template <typename T>
inline T align(T p, uint64_t sz) {
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
inline void setbit(addr_t address, int bit) {
  return set32(address, get32(address) | (1 << bit));
}
inline void clearbit(addr_t address, int bit) {
  return set32(address, get32(address) & (~(1 << bit)));
}
#define SET_CLEAR_BIT(enable, addr, bit) (enable ? setbit : clearbit)(addr, bit)

// ref:
// https://github.com/torvalds/linux/blob/v6.8/arch/arm64/include/asm/sysreg.h#L1117-L1135
#define read_sysreg(r)                         \
  ({                                           \
    uint64_t __val;                            \
    asm volatile("mrs %0, " #r : "=r"(__val)); \
    __val;                                     \
  })
#define write_sysreg(r, v)                           \
  do {                                               \
    uint64_t __val = (uint64_t)(v);                  \
    asm volatile("msr " #r ", %x0" : : "rZ"(__val)); \
  } while (0)

extern "C" {
// start.S
[[noreturn]] void prog_hang();
}
