#ifndef MY_STDLIB_H
#define MY_STDLIB_H

#include "../include/my_stddef.h"

extern int __bss_end;
//have tried 1GB but kernel dead 
#define HEAP_SIZE (1024) // 1 KB  
#define HEAP_START (&__bss_end)
#define HEAP_END ((void *)((uintptr_t)HEAP_START + HEAP_SIZE))


void* alignas(size_t alignment, void* ptr);
void initialize_heap(void);
void* simple_malloc(size_t size);

#endif