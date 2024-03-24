#include "memory.h"
#include "def.h"

static char* heap_ptr = (char*)HEAP;

void* malloc(unsigned long size)
{
    if (!size)
        return NULL;
    size = (unsigned long)mem_align((char*)size, 4);
    if (heap_ptr + size > (char*)HEAP_END)
        return NULL;
    char* ptr = heap_ptr;
    heap_ptr += size;
    return ptr;
}
