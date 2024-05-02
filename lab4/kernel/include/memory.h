#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "u_list.h"

/* Lab2 */
void *allocator(unsigned int size);
// void  free(void* ptr);

/* Lab4 */
#define BUDDY_MEMORY_BASE 0x0           // 0x10000000 - 0x20000000 (SPEC) -> Advanced #3 for all memory region
#define BUDDY_MEMORY_PAGE_COUNT 0x3C000 // let BUDDY_MEMORY use 0x0 ~ 0x3C000000 (SPEC)
#define PAGESIZE 0x1000                 // 4KB

#define CACHE_SEG 0x8
#define CACHE_offset 3
#define CACHE_record_num 8

// #define MAX_PAGES   0x10000    // 65536 (Entries), PAGESIZE * MAX_PAGES = 0x10000000 (SPEC)

typedef enum
{
    FRAME_FREE = -2,
    FRAME_ALLOCATED = -1,
    FRAME_IDX_0 = 0, //  0x1000
    FRAME_IDX_8 = 8,
    FRAME_IDX_FINAL = 17, // 0x20 000 000
    FRAME_MAX_IDX = 18
} frame_value_type;

typedef enum
{
    CACHE_IDX_0 = 0,     //  0x20
    CACHE_IDX_FINAL = 6, // 0x800
    CACHE_MAX_IDX = 7    // 0x1000
} cache_value_type;

typedef struct frame
{
    struct list_head listhead; // store freelist
    int val;                   // store order
    int used;
    unsigned int idx;
} frame_t;

typedef struct cache
{
    struct list_head listhead; // store freelist
    void *data_base;
    int cache_order;
    int max_available;
    int available;
    unsigned long long cache_record[CACHE_record_num];
} cache_t;

void allocator_init();
frame_t *release_redundant(frame_t *frame);
frame_t *get_buddy(frame_t *frame);
int coalesce(frame_t *frame_ptr);

void dump_page_info();
void dump_cache_info();

// buddy system
void *page_malloc(unsigned int size);
void page_free(frame_t *ptr);
void page2caches(int order);
void *cache_malloc(unsigned int size);
void cache_free(void *ptr);

void *kmalloc(unsigned int size);
void kfree(void *ptr);
void memory_reserve(unsigned long long start, unsigned long long end);

void freelist_init();
void page2caches(int c_val);
void *find_CACHE(cache_t* ptr);
frame_t *find_free_page(int val);

#endif /* _MEMORY_H_ */