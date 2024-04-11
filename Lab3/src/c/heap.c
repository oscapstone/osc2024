#include "heap.h"
#include "uart.h"

extern char _heap_top;
static char* htop_ptr = &_heap_top;

void* kmalloc(unsigned int size) {
    // -> htop_ptr
    // htop_ptr + 0x02:  heap_block size
    // htop_ptr + 0x10 ~ htop_ptr + 0x10 * k:
    //            { heap_block }
    // -> htop_ptr

    // 0x10 for heap_block header
    /*char* r = htop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned int*)(r - 0x8) = size;
    htop_ptr += size;
    return r;*/
    void* allocated = (void *)htop_ptr;
	htop_ptr += size;
    /*uart_hex(allocated);
    uart_send('\n');*/
    return allocated;
}

void kfree(void* ptr) {
    // TBD
}
