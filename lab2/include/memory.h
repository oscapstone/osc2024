#pragma once

#include <stddef.h>

#define MEMORY_SIZE 1024

static char memory_pool[MEMORY_SIZE];
static char *memory_ptr = memory_pool;

void *simple_alloc(size_t size)
{
	void *allocated = memory_ptr;

	if (memory_ptr + size - memory_pool > MEMORY_SIZE)
		return NULL;

	memory_ptr += size;

	return allocated;
}
