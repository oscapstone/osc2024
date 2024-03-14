#include "utils.h"

extern int _end;
static char *header;

void heapInit()
{
    header = (char *)&_end;
    header++;
}
void *simpleMalloc(int size)
{
    void *ret = (void *)header;
    header += size;
    return ret;
}