#pragma once
#include "mmio.h"
#include "util.h"

#define GPIO_FSEL_INPUT  0b000
#define GPIO_FSEL_OUTPUT 0b001
#define GPIO_FSEL_ALT0   0b100
#define GPIO_FSEL_ALT1   0b101
#define GPIO_FSEL_ALT2   0b110
#define GPIO_FSEL_ALT3   0b111
#define GPIO_FSEL_ALT4   0b011
#define GPIO_FSEL_ALT5   0b010

#define FSEL_BITS 3
#define FSEL_MASK MASK(FSEL_BITS)

inline void gpio_fsel_set(addr_t addr, int offset, int value) {
  uint32_t temp = get32(addr);
  temp = (temp & ~(FSEL_MASK << offset)) | ((value & FSEL_MASK) << offset);
  set32(addr, temp);
}
