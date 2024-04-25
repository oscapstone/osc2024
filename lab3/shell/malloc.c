#include"header/malloc.h"
extern char __heap_start;
char *top = &__heap_start;
void* simple_malloc(unsigned long size) {
    char* r = top + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned long*)(r - 0x8) = size;
    top += size;
    return r;
}