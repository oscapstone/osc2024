#ifndef _MEMORY_H
#define _MEMORY_H

#define USER_PROGRAM_BASE 0x100000
#define USER_STACK_POINTER_BASE 0x200000

void init_memory();
void *simple_malloc(unsigned int size);
char *get_heap_top();

#endif
