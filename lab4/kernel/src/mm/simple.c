#include <kernel/io.h>
#include <kernel/memory.h>
#include <lib/stdlib.h>

extern int _end;
#define HEAP_MAX (&_end) + 0x10000

static char *heap_top;

void init_memory() {
    heap_top = (&_end);
    // heap_top++;
}

void *simple_malloc(unsigned int size) {
    if (size == 0) {
        return NULL;
    }
    if (heap_top + size >= HEAP_MAX) {
        print_string("Out of memory\n");
        return NULL;
    }
    void *ret = heap_top;
    heap_top += size;
    return ret;
}

char *get_heap_top() { return heap_top; }
