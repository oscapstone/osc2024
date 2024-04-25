#include <stddef.h>

extern char _heap_top;
static char* heaptop_ptr = &_heap_top;

void* malloc(size_t size) {
    // Add heap block header 16 bytes
    char* ptr = heaptop_ptr + 0x10;
    // Size paddling to multiple of 0x10
    size = size + 0x10 - size % 0x10;
    *(unsigned int*)(ptr - 0x8) = size;
    heaptop_ptr += size;    
    return ptr;
}