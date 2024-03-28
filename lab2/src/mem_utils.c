#include "../include/mem_utils.h"

#define HEAP_SIZE 1024

static char HEAP_USE[HEAP_SIZE] = {0};
static unsigned int HEAP_OFFSET = 0;

char *mem_align(char *addr, unsigned int number)
{
    uint64_t x = (uint64_t) addr;
    uint64_t mask = number - 1;
    return (char *)((x + (mask)) & (~mask));
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