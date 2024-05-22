#ifndef _HEAP_H_
#define _HEAP_H_

//lab2
void* malloc(unsigned int size);

//lab4
#include "u_list.h"
#define BUDDY_MEMORY_BASE 0x10000000 // 0x10000000 - 0x20000000 (SPEC)
#define PAGESIZE    0x1000     // 4KB
#define MAX_PAGES   0x10000    // 65536 (Entries), PAGESIZE * MAX_PAGES = 0x10000000 (SPEC)



//order
typedef enum {
    FRAME_FREE = -2,
    FRAME_ALLOCATED = -1,
    FRAME_IDX_0 = 0,      //  0x1000 = 4KB
    FRAME_IDX_1,          //  0x2000
    FRAME_IDX_2,          //  0x4000
    FRAME_IDX_3,          //  0x8000
    FRAME_IDX_4,          // 0x10000
    FRAME_IDX_5,          // 0x20000
    FRAME_IDX_FINAL = 6,  // 0x40000
    FRAME_MAX_IDX = 7
} frame_order_type;

typedef enum {
    CACHE_NONE = -1,     // Cache not used
    CACHE_IDX_0 = 0,     //  0x20
    CACHE_IDX_1,         //  0x40
    CACHE_IDX_2,         //  0x80
    CACHE_IDX_3,         // 0x100
    CACHE_IDX_4,         // 0x200
    CACHE_IDX_5,         // 0x400
    CACHE_IDX_FINAL = 6, // 0x800
    CACHE_MAX_IDX = 7
} cache_order_type;

typedef struct frame
{
    struct list_head listhead; // store freelist
    int order;                   // store order
    int used;
    int cache_order;
    unsigned int idx;
} frame_t;

void init_allocator();
frame_t *release_redundant(frame_t *frame);
frame_t *get_buddy(frame_t *frame);
int coalesce(frame_t **frame_ptr);

void dump_page_info();
void dump_cache_info();

//buddy system
void* page_malloc(unsigned int size);
void  page_free(void *ptr);
void  page2caches(int order);
void* cache_malloc(unsigned int size);
void  cache_free(void* ptr);

void* kmalloc(unsigned int size);
void  kfree(void *ptr);

#endif /* _HEAP_H_ */
