#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "list.h"
#include "stdint.h"

#define ALLOC_BASE          0x0
#define ALLOC_END           0x3B400000
#define QEMU_ALLOC_END      0x3B400000
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

void       *malloc(size_t size);
void       *page_malloc(size_t size);
void        page_free(void* ptr);
void       *kmalloc(size_t size);
void        kfree(void *ptr);
void       *cache_malloc(unsigned int size);
void        cache_free(void *ptr);

void        allocator_init();
frame_t    *get_free_frame(int val);
void        dump_page_info();
void        dump_cache_info();
frame_t    *get_buddy(frame_t *frame);
frame_t    *release_redundant_block(frame_t *ptr);
int         coalesce(frame_t *frame_ptr);

void        page_to_caches(int order);
int         val_to_frame_num(size_t size);
size_t      frame_to_index(frame_t *frame);
frame_t    *index_to_frame(size_t index);
size_t      frame_addr_to_phy_addr(frame_t *frame);
frame_t    *phy_addr_to_frame(void *ptr);

void        memory_reserve(unsigned long long start, unsigned long long end);
void        memory_init();

#endif /* _MEMORY_H_ */