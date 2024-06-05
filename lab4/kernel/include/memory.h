#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "list.h"
#include "stdint.h"

#define MEMORY_BASE         0x0
#define QEMU_MEMORY_BASE    0x3B400000
#define ALLOC_BASE          0x10000000
#define ALLOC_END           0x20000000  
#define PAGE_SIZE           0x1000      // 4kB
#define MAX_PAGE_COUNT      0x10000     // 65536 entries, PAGE_SIZE x MAX_PAGE = 0x10000000 (SPEC)

typedef struct frame {
	struct list_head listhead;
	int val;
    int used;
    int order;
    unsigned int idx;                 
} frame_t;

typedef enum {
    FRAME_VAL_FREE = -2,    // For free frame belongs to a larger contiguous memory block
    FRAME_VAL_USED,         // For allocated frame
    FRAME_IDX_0 = 0,        //  0x1000
    FRAME_IDX_1,            //  0x2000
    FRAME_IDX_2,            //  0x4000
    FRAME_IDX_3,            //  0x8000
    FRAME_IDX_4,            // 0x10000
    FRAME_IDX_5,            // 0x20000
    FRAME_IDX_FINAL = 6,    // 0x40000
    FRAME_MAX_IDX = 7
} frame_val_t;

typedef enum {
    CACHE_NONE = -1,        // Cache not used
    CACHE_IDX_0 = 0,        //  0x20
    CACHE_IDX_1,            //  0x40
    CACHE_IDX_2,            //  0x80
    CACHE_IDX_3,            // 0x100
    CACHE_IDX_4,            // 0x200
    CACHE_IDX_5,            // 0x400
    CACHE_IDX_FINAL = 6,    // 0x800
    CACHE_MAX_IDX = 7
} cache_val_t;

void*   malloc(size_t size);
void    allocator_init();

#endif /* _MEMORY_H_ */