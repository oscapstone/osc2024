#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define HEAP_START 0x20000
#define HEAP_END   0x40000

byteptr_t simple_malloc(uint32_t size);

byteptr_t memory_align(const byteptr_t p, uint32_t s);
int32_t memory_cmp(byteptr_t s1, byteptr_t s2, int n);

#endif