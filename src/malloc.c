#include "malloc.h"
#include "uart.h"

static char *heap_end = (char *) HEAP_START;
void *malloc(unsigned int size)
{
    // size not valid
    if (size <= 0 || size > HEAP_SIZE) {
        uart_puts("size not valid\n");
        return NULL;
    }

    // insufficient heap space
    if ((unsigned long) heap_end + size > HEAP_START + HEAP_SIZE) {
        uart_puts("insufficient heap space\n");
        return NULL;
    }

    char *address = heap_end;
    heap_end += size;
    return (void *) address;
}