#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "type.h"
#include "frame.h"


int32_t     memory_cmp(byteptr_t s1, byteptr_t s2, int32_t n);
byteptr_t   memory_align(const byteptr_t p, uint32_t s);
byteptr_t   memory_padding(const byteptr_t p, uint32_t s);
void        memory_copy(byteptr_t dest, byteptr_t source, int32_t size);
void        memory_zero(byteptr_t dest, int32_t size);

void        memory_system_init(uint32_t start, uint32_t end);
void        memory_print_info();

byteptr_t   memory_alloc(uint32_t size);
byteptr_t   startup_memory_alloc(uint32_t size);

byteptr_t   kmemory_alloc(uint32_t size);
void        kmemory_release(byteptr_t ptr);

#define     malloc      memory_alloc
#define     smalloc     startup_memory_alloc
#define     kmalloc     kmemory_alloc
#define     kfree       kmemory_release

#endif