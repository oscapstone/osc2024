#include "malloc.h"

extern char __heap_start;

char *__heap_top = &__heap_start;

void *simple_malloc(size_t size)
{
    if (size % 8 != 0)
        size += 8 - (size % 8);
    if (__heap_top + size > (char *)0x20000000)
        return 0; // Out of memory

    char *ret = __heap_top;
    __heap_top += size;
    return ret;
}