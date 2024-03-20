#include "../include/heap.h"

extern char _heap_top;
static char *htop_ptr = &_heap_top; // declare it "static" so it'll always remember the last position

void *malloc(unsigned int size)
{
	// -> heap top ptr
	// heap top ptr + 0x02: heap_block size
	// heap top ptr + 0x10 ~ heap top ptr + 0x10 * k:
	//              { heap_block }
	// -> heap top ptr

	// header
	char *r = htop_ptr + 0x10;

	// size padding align to heap_block header
	size = 0x10 + size - size % 0x10;
	// recode the size of space allocated at 0x8 byte before content(heap_block)
	*(unsigned int *)(r - 0x8) = size;
	// mov heap top
	htop_ptr += size;

	return r;
}