#include "memory.h"

#define HEAP_LIMIT (uint32_t)&__end + 0x10000
static char* heap_head = (char*)&__end;

void* simple_alloc(uint32_t size){

    if ((uint32_t)(heap_head + size) > HEAP_LIMIT){
        print_str("\nNot Enough Memory");
        return (char*)0;
    }

    print_str("\nAllocating from 0x");
    print_hex((uint32_t)heap_head);
    print_str(" to ");
    print_hex((uint32_t)heap_head+size);

    print_str("\nLimit Address: 0x");
    print_hex((uint32_t)HEAP_LIMIT);

    char* alloc_tail = heap_head;
    heap_head += size;

    return alloc_tail;
}