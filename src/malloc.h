#ifndef _DEF_MALLOC
#define _DEF_MALLOC

#define NULL         ((void *)0)
#define HEAP_START   0x2000000      // 0x10 breaks on rpi3b+
#define HEAP_SIZE    0x10000

void *malloc(unsigned int);

#endif
