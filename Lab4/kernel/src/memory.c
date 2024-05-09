#include "memory.h"
#include "bool.h"
#include "cpio.h"
#include "dtb.h"
#include "list.h"
#include "mini_uart.h"

extern char heap_begin;
extern char heap_end;

static char* heap_ptr = &heap_begin;

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
