#include "memory.h"
#include "uart.h"

void *simple_malloc(unsigned int size) {
    if ((void*)HEAP_TOP + size > (void*)HEAP_END) {
        uart_puts("Out of memory\n");
        return (char*)0;
    }
    void *alloc_ptr = HEAP_TOP;
    HEAP_TOP += size;
    return alloc_ptr;
}