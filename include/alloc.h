#ifndef _ALLOC_H
#define _ALLOC_H

#include "types.h"

#define SIMPLE_MALLOC_BUFFER_SIZE (1 << 25)

void *simple_malloc(uint32_t size);

#endif