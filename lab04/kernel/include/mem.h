#ifndef __MEM_H__
#define __MEM_H__

#include "type.h"

extern uint64_t _heap_start;

#define HEAP_END            (uint64_t)(&_heap_start + 0x100000)

#define USER_PROCESS_SP     0x200000
#define USER_START_ADDR     0x100000


// malloc
#define FRAME_SIZE          4096    // 4KB
// #define MAX_LEVEL           16
#define MAX_LEVEL           18      // log_2(FRAME_NUM) about 17.~                   

// #define MALLOC_START_ADDR   0x10000000
// #define MALLOC_END_ADDR     0x20000000
#define MALLOC_START_ADDR   0x00000000
#define MALLOC_END_ADDR     0x3C000000
#define MALLOC_TOTAL_SIZE   (MALLOC_END_ADDR - MALLOC_START_ADDR)
#define FRAME_NUM           MALLOC_TOTAL_SIZE / FRAME_SIZE

#define MAX_BUDDY_SYSTEM_LIST 10

#define MEMORY_POOL_SIZE    4
#define MEMORY_POOL_DEPTH   4

#endif