#include "../include/mem_utils.h"

#define HEAP_SIZE 1024

static char HEAP_USE[HEAP_SIZE] = {0};
static unsigned int HEAP_OFFSET = 0;

void mem_align(void *addr, unsigned int number)
{
    unsigned long *x = (unsigned long *)addr;
    unsigned long mask = number - 1;
    *x = (*(x) + mask) & (~mask);
}

void *malloc(unsigned int size)
{
    if (size + HEAP_OFFSET >= HEAP_SIZE) {
        return NULL;
    }
    unsigned int now_index = HEAP_OFFSET;
    HEAP_OFFSET += size;
    return &HEAP_USE[now_index];
}