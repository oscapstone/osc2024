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

typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef volatile uint64_t addr_t;

// util-asm.S
void set32(addr_t address, uint32_t value);
uint32_t get32(addr_t address);
void wait_cycle(unsigned cycle);

// start.S
void prog_hang();
