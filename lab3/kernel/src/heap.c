#include "heap.h"

extern char _heap_top;
static char* htop_ptr = &_heap_top;

void* kmalloc(unsigned int size) {
    // -> htop_ptr
    //               header 0x10 bytes                   block    
    // |--------------------------------------------------------------|
    // |  fill zero 0x8 bytes | size 0x8 bytes | size padding to 0x16 |
    // |--------------------------------------------------------------|
    
    // 0x10 for heap_block header
    char* r = htop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned int*)(r - 0x8) = size;
    htop_ptr += size;
    return r;
}


void free(void* ptr) {
    // TBD
}
