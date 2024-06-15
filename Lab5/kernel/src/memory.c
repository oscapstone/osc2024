#include "memory.h"
#include "bool.h"
#include "cpio.h"
#include "def.h"
#include "dtb.h"
#include "list.h"
#include "mini_uart.h"
#include "utils.h"

static char* heap_ptr = (char*)HEAP_START;

int mem_init(uintptr_t dtb_ptr)
{
    /* dtb addreses */
    if (fdt_init(dtb_ptr))
        goto fail;

    /* cpio addresses */
    if (cpio_init())
        goto fail;

    /* usable memory region */
    // find the #address-cells and #size-cells (for reg property of the children
    // node), and use this information to extract the reg property of memory
    // node.
    if (fdt_traverse(fdt_find_root_node) || fdt_traverse(fdt_find_memory_node))
        goto fail;

    return 1;

fail:
    return 0;
}

void* mem_alloc(uint64_t size)
{
    if (!size)
        return NULL;

    size = (uint64_t)mem_align((char*)size, 8);

    if (heap_ptr + size > (char*)HEAP_END)
        return NULL;

    char* ptr = heap_ptr;
    heap_ptr += size;

    return ptr;
}

void* mem_alloc_align(uint64_t size, uint32_t align)
{
    if (!size)
        return NULL;
    if (!align)
        align = 8;

    heap_ptr = mem_align(heap_ptr, align);
    return mem_alloc(size);
}


/*
 * with `-nostdlib` compiler flag, we have to implement this function since
 * compiler may generate references to this function
 */
void memset(void* b, int c, size_t len)
{
    size_t offset = mem_align(b, sizeof(void*)) - b;
    len -= offset;
    size_t word_size = len / sizeof(void*);
    size_t byte_size = len % sizeof(void*);
    unsigned char* offset_ptr = (unsigned char*)b;
    unsigned long* word_ptr = (unsigned long*)(offset_ptr + offset);
    unsigned char* byte_ptr = (unsigned char*)(word_ptr + word_size);
    unsigned char ch = (unsigned char)c;
    unsigned long byte = (unsigned long)ch;
    unsigned long word = (byte << 56) | (byte << 48) | (byte << 40) |
                         (byte << 32) | (byte << 24) | (byte << 16) |
                         (byte << 8) | byte;

    for (size_t i = 0; i < offset; i++)
        offset_ptr[i] = ch;

    for (size_t i = 0; i < word_size; i++)
        word_ptr[i] = word;

    for (size_t i = 0; i < byte_size; i++)
        byte_ptr[i] = ch;
}

/*
 * with `-nostdlib` compiler flag, we have to implement this function since
 * compiler may generate references to this function
 */
void* memcpy(void* dst, const void* src, size_t n)
{
    char* dst_cp = dst;
    char* src_cp = (char*)src;
    for (size_t i = 0; i < n; i++)
        dst_cp[i] = src_cp[i];

    return dst;
}
