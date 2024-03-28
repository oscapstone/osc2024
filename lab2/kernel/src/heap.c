#include "heap.h"

extern char _heap_top;
static char* htop_ptr = &_heap_top;

void* malloc(unsigned int size) {
    // 0x10 for heap_block header
    char* r = htop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned int*)(r - 0x8) = size;
    int total_size = size + 0x10;
    htop_ptr += total_size;
    
    return r;
}
