#include "memory.h"

byteptr_t
memory_align(const byteptr_t p, uint32_t s) {
	uint64_t x = (uint64_t) p;
	return (byteptr_t) ((x + s - 1) & (~(s - 1)));
}


int32_t
memory_cmp(byteptr_t s1, byteptr_t s2, int32_t n)
{
    byteptr_t a = s1, b = s2;
    while (n-- > 0) { if (*a != *b) { return *a - *b; } a++; b++; }
    return 0;
}

static byteptr_t malloc_cur = (byteptr_t) HEAP_START;

byteptr_t 
simple_malloc(uint32_t size)
{
    size = (uint32_t) memory_align((byteptr_t) (0l | size), 4);
    if ((uint32_t) (malloc_cur + size) > HEAP_END) return 0;
    byteptr_t ret_ptr = malloc_cur;
    malloc_cur += size;
    return ret_ptr;
}