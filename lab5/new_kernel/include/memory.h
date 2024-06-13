#include "utility.h"
#ifndef _MEMORY_H
#define _MEMORY_H

#define BUDDY_SYSTEM_BASE 0x0 //0x1000000    // for basic 1
#define WHOLE_MEMORY 0x3B400000
#define BUDDY_SYSTEM_PAGE_COUNT 0x3B400 // for basic 1

#define PAGE_SIZE 0x1000 // 4kB
#define CACHE_SIZE 0x10  // 16Bytes

enum results
{
    Fault = 0,
    Success,
};

enum page_status
{
    FREE,
    isBelonging,
    Allocated,
};

typedef enum
{
    FRAME_IDX_0 = 0,     //  0x1000
    FRAME_IDX_1,         //  0x2000
    FRAME_IDX_2,         //  0x4000
    FRAME_IDX_3,         //  0x8000
    FRAME_IDX_4,         // 0x10000
    FRAME_IDX_5,         // 0x20000
    FRAME_IDX_FINAL = 6, // 0x40000
    FRAME_MAX_IDX = 7
} page_size_order;

typedef enum
{
    CACHE_NONE = -1,    // Cache not used
    CACHE_IDX_0 = 0,    //  0x10 256 chunks
    CACHE_IDX_1,        //  0x20 128 chunks
    CACHE_IDX_2,        //  0x40 64 chunks
    CACHE_IDX_3,        //  0x80 32 chunks
    CACHE_IDX_4,        // 0x100 16 chunks
    CACHE_IDX_5,        // 0x200 8 chunks
    CACHE_IDX_6,        // 0x400 4 chunks
    CACHE_IDX_FINAL = 7 // 0x800 2 chunks
} cache_size_order;

typedef struct page_frame
{
    /* data */
    struct list_head listhead;
    enum page_status status;
    int order;
    unsigned int idx;
    int cache_order;
    int cache_used_count;
} page_frame_t;

// lab 4 basic 1
void init_memory_space();
void *page_alloc(unsigned int size);
void page_free(void *pg_ptr);
enum results buddy_can_merge(page_frame_t *pg_ptr);
void pg_info_dump();
page_frame_t *split_to_target_freelist(int order);
page_frame_t *find_buddy(page_frame_t *pg_ptr);
page_frame_t *merge(page_frame_t *pg_ptr);

// lab 4 basic 2
void *cache_alloc(unsigned int size);
void cache_free(void *cache_ptr);
void page_to_cache_pool(int cache_order);
void cache_info_dump();

// lab 4 adv 2
void *kmalloc(unsigned int size);
void kfree(void *ptr);
void memory_reserve(unsigned long long start,unsigned long long end);

#endif