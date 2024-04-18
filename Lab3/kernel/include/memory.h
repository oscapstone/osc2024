#ifndef MEMORY_H
#define MEMORY_H

#include "mm.h"

extern void* mem_align(void*, unsigned long);
void* mem_alloc(unsigned long);
void mem_free(void*);

#endif /* MEMORY_H */
