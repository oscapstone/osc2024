#include "malloc.h"
#include "mini_uart.h"

extern char __end__, __HeapLimit;
static char *heapStartAddr = &__end__;
static char *heapEndAddr = &__HeapLimit;

void* my_malloc(int msize) {
    int avail_size = heapEndAddr - heapStartAddr;
    if(msize > avail_size) {
        uart_send_string("heap space running out\r\n");
        return (void*) 0;
    }
    char *ptr = heapStartAddr;
    heapStartAddr += msize;
    return ptr;
}