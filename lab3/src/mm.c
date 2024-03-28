#include "mm.h"
#include "mini_uart.h"

extern char _end;
char* __heap_ptr = (char *)&_end;

void* simple_malloc(unsigned int bytes){
    if(__heap_ptr + bytes >= HEAP_MAX){
        debug("Heap overflow");
        return (void *)0;
    }
    void *ret = __heap_ptr;
    __heap_ptr += bytes;
    return ret;
}
