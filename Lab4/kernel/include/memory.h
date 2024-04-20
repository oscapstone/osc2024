#ifndef MEMORY_H
#define MEMORY_H

#include "int.h"
#include "mm.h"

extern void* mem_align(void*, uint64_t);
void* mem_alloc(uint64_t);
void mem_free(void*);

#endif /* MEMORY_H */
