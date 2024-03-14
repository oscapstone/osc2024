#include "stdlib.h"
#include "stddef.h"

volatile unsigned long available = 0;

void *simple_malloc(size_t size)
{
    if (BASE + available > LIMIT)
        return NULL;
    void * returned_pointer = (void *) (BASE + available);
    available += size;
    return returned_pointer;
}

int return_available()
{
    return (int) BASE + available;
}