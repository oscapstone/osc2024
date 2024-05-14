#include <kernel/io.h>
#include <kernel/memory.h>
#include <lib/stdlib.h>

extern uint64_t __heap_start;
#define HEAP_MAX (&__heap_start) + 0x100000

static char *heap_top;

void init_memory() {
    heap_top = (char *)(&__heap_start);
    // heap_top++;
}

void *simple_malloc(unsigned int size) {
    if (size == 0) {
        return NULL;
    }
    if (heap_top + size >= (char *)HEAP_MAX) {
        print_string("Out of memory\n");
        return NULL;
    }
    void *ret = heap_top;
    heap_top += size;
    return ret;
}

char *get_heap_top() { return heap_top; }
