#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "type.h"

#define HEAP_START      0x20000
#define HEAP_END        0x40000

byteptr_t   memory_alloc(uint32_t size);

byteptr_t   memory_align(const byteptr_t p, uint32_t s);
int32_t     memory_cmp(byteptr_t s1, byteptr_t s2, int32_t n);

#define     malloc      memory_alloc

#endif