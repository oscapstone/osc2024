#ifndef STDLIB_H
#define STDLIB_H



typedef unsigned long size_t;

#define BASE  ((volatile void *)(0x60000))
#define LIMIT ((volatile void *)(0x7FFFF))

extern volatile unsigned long available;

void *simple_malloc(size_t size);
long return_available();

#endif