#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "utility.h"
#include "stdint.h"

#define PAGE_FRAME_SHIFT 12 // 4KB
#define PAGE_FRAME_SIZE (1 << PAGE_FRAME_SHIFT)
#define MAX_VAL 17 // max buddy frame size: 4KB * 2^6
// min cache size: 2^(5) = 32B
// max cache size: 2^(5 + MAX_ORDER) = 2KB
#define BASE_ORDER 5
#define MAX_ORDER 6
#define NOT_CACHE -1

typedef struct frame
{
	struct list_head listhead;
	int8_t val;
	int8_t order;			  // 2^(BASE_ORDER + order) = size
	uint8_t cache_used_count; // 4KB / 2^(BASE_ORDER) = 128. max count is 128. Max num of type is 256
} frame_t;

void init_memory_space();
void *kmalloc(long size);
void kfree(void *ptr);
long get_memory_size();
void init_cache();
int memory_reserve(long start, long end);

void *page_malloc(long size);
int page_free(void *frame);

frame_t *get_free_frame(int val);
frame_t *split_frame(int8_t val);

#endif /* _MEMORY_H_ */
