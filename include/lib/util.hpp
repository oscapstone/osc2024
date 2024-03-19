#pragma once

#define MASK(bits) ((1 << bits) - 1)
#define NOP        asm volatile("nop")

// from v8/v8:src/base/compiler-specific.h#L21-L31
#if defined(__GNUC__)
#define PRINTF_FORMAT(format_param, dots_param) \
  __attribute__((format(printf, format_param, dots_param)))
#else
#define PRINTF_FORMAT(format_param, dots_param)
#endif

using uint32_t = unsigned int;
using uint64_t = unsigned long;
using addr_t = volatile char*;

// util-asm.S
extern "C" {
void set32(addr_t address, uint32_t value);
uint32_t get32(addr_t address);
void wait_cycle(unsigned cycle);

// start.S
void prog_hang();
}
