#pragma once

#define MASK(bits) ((1 << bits) - 1)
#define NOP        asm volatile("nop")

typedef unsigned int uint32_t;
typedef volatile long addr_t;

void set32(addr_t address, uint32_t value);
uint32_t get32(addr_t address);
void wait_cycle(unsigned cycle);
