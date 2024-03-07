#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "kernel/utils.h"
// Get the symbol __end from linker script
extern char* __end;
// make allocated variable global among all files
extern char* allocated;
// return requested 'size' bytes which are continuous space
void* simple_malloc(unsigned int size);

#endif