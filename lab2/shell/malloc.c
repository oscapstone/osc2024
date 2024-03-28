#include"header/malloc.h"
extern __heap_start;
char *top = &__heap_start;
void* simple_malloc(unsigned int size) {
    return top += size;
}