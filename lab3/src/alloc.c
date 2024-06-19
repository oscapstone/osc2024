#include "alloc.h"
#include "mini_uart.h"

extern int bss_end;                 // `bss_end` is defined in linker script
void *heap_top;

void allocator_init()               // Set heap base address
{
    heap_top = (void *)&bss_end;
    // heap_top++;
}

void *simple_malloc(int size)
{
    if (size < 0)
        return 0;
    
    int align_size = (size/16 + (size%16 ? 1 : 0))*16;

    // uart_send_string("original heap top: 0x");
    // uart_send_string_int2hex((int)(unsigned int *)heap_top);
    // uart_send_string("\r\n");

    void *malloc_begin = heap_top;
    heap_top += align_size;
  
    // uart_send_string("malloc begin address: 0x");
    // uart_send_string_int2hex((int)(unsigned int *)malloc_begin);
    // uart_send_string("\r\n");
    // uart_send_string("malloc size: 0x");
    // uart_send_string_int2hex(size);
    // uart_send_string("\r\n");
    // uart_send_string("current heap top: 0x");
    // uart_send_string_int2hex((int)(unsigned int *)heap_top);
    // uart_send_string("\r\n");

    return malloc_begin;
}