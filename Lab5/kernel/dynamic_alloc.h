#ifndef _DYNAMIC_ALLOC_H_
#define _DYNAMIC_ALLOC_H_

#include "buddy_system_2.h"

#define USED_SLOT 'U'
#define FREE_SLOT 'F'

typedef struct _slab_cache {
    entry_t* head;
    entry_t* tail;
} slab_cache_t;

void startup_allocator(void);
void init_dynamic_alloc(void);
uint64_t dynamic_alloc(uint64_t request);
void dynamic_free(uint64_t addr);

#endif