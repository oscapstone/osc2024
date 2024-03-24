#ifndef MEMORY_H
#define MEMORY_H

#include "mm.h"

/* 1MB HEAP */
#define HEAP     (LOW_MEMORY - 0x200000)  // 0x200000
#define HEAP_END (LOW_MEMORY - 0x100000)  // 0x300000

extern void* mem_align(void*, unsigned long);
void* malloc(unsigned long);

#endif /* MEMORY_H */
