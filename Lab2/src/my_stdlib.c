#include "../include/my_stdlib.h"
#include "../include/my_stddef.h"
#include "../include/my_stdint.h"
#include "../include/uart.h"


static uintptr_t heap_index = (uintptr_t)HEAP_START;

void* alignas(size_t alignment, void* ptr) {
    uintptr_t ptr_value = (uintptr_t)ptr;
    uintptr_t mask = alignment - 1;
    return (void*)((ptr_value + mask) & ~mask);
}

void initialize_heap() {
    // Initialize heap index
    heap_index = (uintptr_t)HEAP_START;
    
}

void *simple_malloc(size_t size) {
    // Align size to multiple of 8
    size = (size + 7) & ~7;

    // Check if there's enough space in the heap
    if (heap_index + size > (uintptr_t)HEAP_END) {
        uart_puts("Error: Insufficient space in the heap\n");
        return NULL; // Out of memory
    }

    // Allocate memory from the heap
    void *ptr = (void *)heap_index;
    heap_index += size;
    return ptr;
}