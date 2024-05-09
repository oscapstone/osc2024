#ifndef MEMORY_H
#define MEMORY_H

#include "def.h"
#include "int.h"
#include "mm.h"

extern void* mem_align(void*, uint64_t);
int mem_init(uintptr_t dtb_ptr);
void* mem_alloc(uint64_t);
void mem_free(void*);

#endif /* MEMORY_H */
