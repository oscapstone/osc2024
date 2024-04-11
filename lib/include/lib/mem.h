#ifndef _MEM_H
#define _MEM_H

#include "int.h"

void *malloc(size_t size);

void memset(void *src, int c, size_t n);

void memcpy(void *dest, const void *src, size_t n);

#endif  // _MEM_H
