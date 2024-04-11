#include "memory.h"
#include "uart.h"

void *simple_malloc(unsigned int size) {
    if ((void*)HEAP_TOP + size > (void*)HEAP_END) {
        uart_puts("Out of memory\n");
        return (char*)0;
    }
    uart_puts("Allocating memory from ");
    uart_hex((unsigned int)HEAP_TOP);
    uart_puts(" to ");
    void *alloc_ptr = HEAP_TOP;
    HEAP_TOP += size;
    uart_hex((unsigned int)HEAP_TOP);
    return alloc_ptr;
}