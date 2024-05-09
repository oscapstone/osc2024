#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "dlist.h"
#include "types.h"

#define SMALL_SIZES_COUNT 6
#define SMALL_SIZE 32

void memory_pool_init();
int memory_pool_find_pool_index(uint32_t size);
void *memory_pool_allocator(uint32_t size);
void memory_pool_free(void *address);

#endif /* ALLOCATOR_H */