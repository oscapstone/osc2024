#ifndef _ALLOC_H_
#define _ALLOC_H_

#include "../peripherals/utils.h"
#include <stdint.h>

#define HEAP_SIZE 1024 * 128 // Define 128KB heap size.
#define ALIGNMENT 8         // Align the heap pointer to the multiple of 8 before allocating.
#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

static unsigned char heap[HEAP_SIZE];
static unsigned char* heap_ptr;

void init_heap();
void* simple_malloc(size_t size);

#endif