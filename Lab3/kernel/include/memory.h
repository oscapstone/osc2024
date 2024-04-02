#ifndef MEMORY_H
#define MEMORY_H

#include "mm.h"

extern void* mem_align(void*, unsigned long);
void* mem_alloc(unsigned long);

#endif /* MEMORY_H */
