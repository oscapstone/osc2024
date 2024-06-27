#include "../include/my_stdlib.h"

char *heap_curr = (char*)&__end;
int offset=0;

void* alignas(size_t alignment, void* ptr) {
    uintptr_t ptr_value = (uintptr_t)ptr;
    uintptr_t mask = alignment - 1;
    return (void*)((ptr_value + mask) & ~mask);
}

void *simple_malloc(size_t size) {
    // Align size to multiple of 8
    size = (size + 7) & ~7; 

    // Check if there's enough space in the heap
    if (offset + size > MAX_HEAP_SIZE) {
        uart_puts("Error: Insufficient space in the heap\n");
        return NULL; // Out of memory
    }

    // Allocate memory from the heap
    void *ptr = (void *)heap_curr;
    heap_curr += size;
    offset += size;

    return ptr;
}


int log(int num, int base){
    int i=0;
    while(num!=1){
        num/=base;
        i++;
    }
    return i;
}
