#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "list.h"
#include "stddef.h"

#define PAGE_FRAME_SHIFT 12 // 4KB
#define PAGE_FRAME_SIZE (1 << PAGE_FRAME_SHIFT)
#define MAX_VAL 18 // max buddy frame size: 4KB * 2^6

typedef struct frame
{
	struct list_head listhead;
	int val;
} frame_t;

void *kmalloc(size_t size);
void kfree(void *ptr);
size_t get_memory_size();
void memory_init();

int allocate_frame();
int allocate_frame_buddy(int begin_frame, int count, int val);

void *page_malloc(size_t size);
int page_free(void *frame);
int page_insert(int val, frame_t *frame);
frame_t *get_buddy(int val, frame_t *frame);

void *frame_addr_to_phy_addr(frame_t *frame);
frame_t *phy_addr_to_frame(void *ptr);
size_t frame_to_index(frame_t *frame);
frame_t *index_to_frame(size_t index);

void dump_frame();

#endif /* _MEMORY_H_ */
