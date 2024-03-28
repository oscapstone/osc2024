#include "allocator.h"

volatile unsigned char *heap_ptr = ((volatile unsigned char *)(0x70000));

void* simple_malloc(unsigned long long size)
{
    void* re_ptr = (void *)heap_ptr;
    heap_ptr += size;

    return re_ptr;
}