#include "headers/allocator.h"
#include "headers/utils.h"

extern char __heap_head;
char *heap_tail = (char *) &__heap_head;

void* simple_malloc(unsigned int size) 
{
    char *r = heap_tail;
    heap_tail += size;
    return r;
}