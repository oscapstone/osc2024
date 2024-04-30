#ifndef __ALLOC_H__
#define __ALLOC_H__

#include "type.h"

void  mem_init();
void* simple_malloc(uint32_t size);

struct frame_t;
struct free_t;
struct memory_pool_t;
struct chunk_t;

void frame_init_with_reserve();

void* balloc(uint64_t size);
int bfree(void* ptr);

void memory_pool_init();
void* dynamic_alloc(uint64_t size);
int dfree(void* ptr);

void memory_reserve(uint64_t start, uint64_t end, char* msg);

#endif