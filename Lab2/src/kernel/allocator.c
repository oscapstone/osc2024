#include "kernel/allocator.h"
// get the heap address
char *allocated = (char*)&__end;

void* simple_malloc(unsigned int size){
    // 64bits=8bytes
    size += align_offset(size, 8);
    allocated += size;

    return allocated;
}