#include "malloc.h"

extern char __heap_top;
static char *_curr_top = &__heap_top;

void *mymalloc(size_t size)
{
    /*
        0  ~   7 : size
        8 ~ size : data & padding
    */

    void *ret = _curr_top;

    size = size + (size % 0x10 == 0 ? 0 : 0x10 - size % 0x10);
    *(unsigned int*)ret = size;
    _curr_top += size + 8;
    return (ret + 8);
}
