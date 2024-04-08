#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "kernel/utils.h"

#define MAX_HEAP_SIZE 8192
// Get the symbol __end from linker script
extern char* __end;
// make allocated variable global among all files
extern char* allocated;
extern int offset;
// return requested 'size' bytes which are continuous space
void* simple_malloc(unsigned int size);

#endif