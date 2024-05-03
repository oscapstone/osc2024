#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "type.h"

int32_t     memory_cmp(byteptr_t s1, byteptr_t s2, int32_t n);
byteptr_t   memory_align(const byteptr_t p, uint32_t s);
byteptr_t   memory_padding(const byteptr_t p, uint32_t s);

byteptr_t   memory_alloc(uint32_t size);
byteptr_t   simple_memory_alloc(uint32_t size);

void        memory_reserve(byteptr_t start, byteptr_t end);

#define     malloc      memory_alloc
#define     smalloc     simple_memory_alloc

void        memory_system_init(uint32_t start, uint32_t end);


// typedef byteptr_t (*memseg_cb) (byteptr_t ptr, uint32_t value);

// void memseg_process(memseg_cb cb, byteptr_t start_ptr, uint32_t total_count);


#endif