#ifndef MY_STDLIB_H
#define MY_STDLIB_H

//#include "../include/my_stddef.h"
//#include "../include/my_stdint.h"

#include "../include/my_stddef.h"
#include "../include/my_stdint.h"
#include "../include/uart.h"

extern char* __end;

#define MAX_HEAP_SIZE (1024*1024)

#define read_register(sys) ({   \
    uint64_t _val;            \
    asm volatile ("mrs %0, " #sys : "=r" (_val)); \
    _val;                     \
})

#define write_register(sys, _val) ({    \
    asm volatile ("msr " #sys ", %0" :: "r" (_val)); \
})


void* alignas(size_t alignment, void* ptr);
void* simple_malloc(size_t size);
int log(int num, int base);

#endif

