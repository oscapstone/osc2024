#include "malloc.h"

extern char _heap_start;
static char* ptr = &_heap_start;

void* malloc(uint32_t size) {
    char* ret = ptr + sizeof(malloc_chunk);

    // The smallest chunk is 0x20 -> at least 0x18 can be used
    if (size < 0x18) size = 0x18;

    // 0x10 alignment
    size = (size + 0x17) & 0xfffffff0;

    ((malloc_chunk*)ret)->chunk_size = size;

    ptr += size;

    return ret;
}