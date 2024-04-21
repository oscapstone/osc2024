#include "memory.h"
#include "def.h"
#include "dtb.h"

extern char heap_begin;
extern char heap_end;

static char* heap_ptr = &heap_begin;

/* Get from device tree */
// usable_mem_start;   // 0x0
// usable_mem_length;  // 0x3c000000

/* reserved memory region */
#define DTS_MEM_RESERVED_START 0x0
#define DTS_MEM_RESERVED_LENGTH 0x1000;
// cpio_start - cpio_end
// dtb_start - dtb_end
// kernel_start - kernel_end
// startup allocator

// void mem_init()

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

// void* page_frame_alloc() {}
