#include "alloc.h"
#include "mini_uart.h"

// Initialize the heap pointer.
void init_heap() {
    // uart_send_string("Heap start\r\n");
    // uart_send_uint((uintptr_t)heap);
    // uart_send_string("\r\n");
    heap_ptr = (unsigned char *)ALIGN((uintptr_t)heap, ALIGNMENT);
    // uart_send_string("After heap alignment\r\n");
    // uart_send_uint((uintptr_t)heap_ptr);
    // uart_send_string("\r\n");
}

// A simple allocater for continuous space. No handler was implemented, so the user
// must be careful when using the allocated memory space. No warnings will be given if
// accessing memory space not allocated.
// For example, int* table = malloc(sizeof(int) * 8), table[8] = 6. 8 exceeds allocated
// memory.
void* simple_malloc(size_t size) {
    heap_ptr = (unsigned char *)ALIGN((uintptr_t)heap_ptr, ALIGNMENT);

    if (heap_ptr + size > heap + HEAP_SIZE) {
        // Used all of heap's memory.
        uart_send_string("Heap memory exhausted! Failed to allocate memory :(\r\n");
        return NULL;
    }

    // uart_send_string("ptr address\r\n");
    // uart_send_uint((uintptr_t)heap_ptr);
    // uart_send_string("\r\n");

    void* ptr = heap_ptr;
    
    heap_ptr += size;
    // uart_send_string("ptr address\r\n");
    // uart_send_uint((uintptr_t)ptr);
    // uart_send_string("\r\n");

    return ptr;
}