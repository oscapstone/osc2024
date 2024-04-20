#include "memory.h"
#include "def.h"
#include "dtb.h"

extern char heap_begin;
extern char heap_end;

static char* heap_ptr = &heap_begin;

extern uintptr_t usable_mem_start;
extern uintptr_t usable_mem_length;

#define DTS_MEM_RESERVED_START 0x0
#define DTS_MEM_RESERVED_LENGTH 0x1000;

void* mem_alloc(uint64_t size)
{
    if (!size)
        return NULL;
    size = (uint64_t)mem_align((char*)size, 8);
    if (heap_ptr + size > &heap_end)
        return NULL;
    char* ptr = heap_ptr;
    heap_ptr += size;
    return ptr;
}

void mem_free(void* ptr)
{
    // TODO
    return;
}
