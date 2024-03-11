#pragma once

#define MASK(bits) ((1 << bits) - 1)
#define NOP        asm volatile("nop")

typedef unsigned int uint32_t;
typedef volatile long addr_t;

// util-asm.S
void set32(addr_t address, uint32_t value);
uint32_t get32(addr_t address);
void wait_cycle(unsigned cycle);

// util.c
void memzero(void* start, void* end);
int strcmp(const char* s1, const char* s2);
