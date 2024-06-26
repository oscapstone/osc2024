#ifndef MEMORY_H
#define MEMORY_H

#include "def.h"
#include "int.h"
#include "mm.h"

#define STACK_END   LOW_MEMORY + VA_START
#define STACK_START (STACK_END - 0x200000)

#define HEAP_START LOW_MEMORY + VA_START
#define HEAP_END   (HEAP_START + 0xF08000)

extern void* mem_align(void*, uint64_t);
int mem_init(uintptr_t dtb_ptr);
void* mem_alloc(uint64_t size);
void* mem_alloc_align(uint64_t size, uint32_t align);
void memset(void* b, int c, size_t len);
void* memcpy(void* dst, const void* src, size_t n);

#endif /* MEMORY_H */
