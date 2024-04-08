#ifndef __ALLOCATOR_H
#define __ALLOCATOR_H

extern volatile unsigned char *heap_ptr;

void* simple_malloc(unsigned long long size);

#endif