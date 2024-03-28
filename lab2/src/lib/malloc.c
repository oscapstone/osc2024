#include "malloc.h"

extern char __heap_start;

char *__heap_top = &__heap_start;

void *simple_malloc(unsigned long size)
{
    if (size % 8 != 0)
        size += 8 - (size % 8);

    char *ret = __heap_top;
    __heap_top += size;
    return ret;
}