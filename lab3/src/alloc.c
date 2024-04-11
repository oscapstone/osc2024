#include "alloc.h"

// `bss_end` is defined in linker script
extern int __heap_top;

static char *heap_top;

// Set heap base address
void alloc_init()
{
	heap_top = (char *)&__heap_top;
}

void *simple_malloc(int size)
{
	void *p = (void *)heap_top + 0x10;
	// If requested memory size < 0, return NULL
	size = 0x10 + size - size % 0x10;
	*(unsigned int*)(p - 0x8) = size;
	heap_top += size;
	return p;
}