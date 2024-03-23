#ifndef MEM_UTILS_H
#define MEM_UTILS_H

#define NULL 0

void mem_align(void *addr, unsigned int number);
void *malloc(unsigned int size);

#endif