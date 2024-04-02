#include "memory.h"
#include "def.h"

extern char heap_begin;
extern char heap_end;

static char* heap_ptr = &heap_begin;

void* mem_alloc(unsigned long size)
{
    if (!size)
        return NULL;
    size = (unsigned long)mem_align((char*)size, 4);
    if (heap_ptr + size > &heap_end)
        return NULL;
    char* ptr = heap_ptr;
    heap_ptr += size;
    return ptr;
}
