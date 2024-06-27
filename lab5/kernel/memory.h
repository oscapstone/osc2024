#ifndef _MEMORY_H
#define _MEMORY_H

#define USER_PROGRAM_BASE 0x100000
#define USER_STACK_POINTER_BASE 0x200000

#include <lib/stddef.h>

void init_memory();
void *simple_malloc(unsigned int size);
char *get_heap_top();

void init_kmalloc();
void *kmalloc(uint32_t size);
void kfree(void *ptr);

void print_kmalloc_caches();
void print_free_areas();

#endif
