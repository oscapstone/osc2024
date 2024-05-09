#include "stdlib.h"
#include "stddef.h"

unsigned long available = 0;

void *simple_malloc(size_t size)
{
    if (BASE + available + size > LIMIT)
        return NULL;
    void *returned_pointer = (void *) (BASE + available);
    available += size;
    return (void *) returned_pointer;
}

/* return available address */
long return_available()
{
    return (long) BASE + available;
}