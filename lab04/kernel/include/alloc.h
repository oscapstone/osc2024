#ifndef __ALLOC_H__
#define __ALLOC_H__

#include "type.h"

void  mem_init();
void* simple_malloc(uint32_t size);

struct frame_t;
struct free_t;
struct memory_pool_t;

void frame_init();

void* balloc(uint64_t size);

void* dynamic_alloc(uint64_t size);
int dfree(void* ptr);

#endif