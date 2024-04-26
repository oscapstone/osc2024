#include "alloc.h"
#include "mem.h"

static char* HEAP_HEADER;
static char* HEAP_REAR;

void  mem_init()
{
    HEAP_HEADER = (char*)&_end;
    HEAP_REAR = HEAP_HEADER;
}

void* simple_malloc(uint32_t size)
{
    if((void*)HEAP_REAR + size > (void*)HEAP_END){

        return (char*)0;
    }
    char* ret = HEAP_REAR;
    HEAP_REAR += size;
    return ret;
}