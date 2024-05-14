#ifndef MEM_UTILS_H
#define MEM_UTILS_H

#include <stdint.h>
#define NULL 0

char *mem_align(char *addr, unsigned int number);
void *malloc(unsigned int size);


#endif