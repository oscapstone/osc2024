#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "u_list.h"

/* Lab2 */
void* s_allocator(unsigned int size);
void  s_free(void* ptr);

/* Lab4 */
#define BUDDY_MEMORY_BASE       0x0     // 0x10000000 - 0x20000000 (SPEC) -> Advanced #3 for all memory region
#define BUDDY_MEMORY_PAGE_COUNT 0x3C000 // let BUDDY_MEMORY use 0x0 ~ 0x3C000000 (SPEC)
#define PAGESIZE    0x1000     // 4KB
#define MAX_PAGES   0x10000    // 65536 (Entries), PAGESIZE * MAX_PAGES = 0x10000000 (SPEC)

typedef enum {
    FRAME_FREE = -2,
    FRAME_ALLOCATED,
    FRAME_IDX_0 = 0,      //  0x1000
    FRAME_IDX_1,          //  0x2000
    FRAME_IDX_2,          //  0x4000
    FRAME_IDX_3,          //  0x8000
    FRAME_IDX_4,          // 0x10000
    FRAME_IDX_5,          // 0x20000
    FRAME_IDX_FINAL = 6,  // 0x40000
    FRAME_MAX_IDX = 7
} frame_value_type;

typedef enum {
    CHUNK_NONE = -1,     // Chunk not used
    CHUNK_IDX_0 = 0,     //  0x20
    CHUNK_IDX_1,         //  0x40
    CHUNK_IDX_2,         //  0x80
    CHUNK_IDX_3,         // 0x100
    CHUNK_IDX_4,         // 0x200
    CHUNK_IDX_5,         // 0x400
    CHUNK_IDX_FINAL = 6, // 0x800
    CHUNK_MAX_IDX = 7
} chunk_value_type;

typedef struct frame
{
    struct list_head listhead; // store freelist
    int val;                   // store order
    int used;
    int chunk_order;
    unsigned int idx;
} frame_t;

void     init_allocator();
frame_t *release_redundant(frame_t *frame);
frame_t *get_buddy(frame_t *frame);
int      coalesce(frame_t *frame_ptr);

void page_info();
void chunk_info();

//buddy system
void* page_malloc(unsigned int size);
void  page_free(void *ptr);
void  page2chunks(int order);
void* chunk_malloc(unsigned int size);
void  chunk_free(void* ptr);

void* kmalloc(unsigned int size);
void  kfree(void *ptr);
void  memory_reserve(unsigned long long start, unsigned long long end);
int all_chunks_free_in_page(frame_t* page_frame);
void print_allocated_pages_addr();
void print_allocated_chunks_addr();
#endif /* _MEMORY_H_ */
