#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "list.h"

#define PAGE_FRAME_SHIFT 12 // 4KB
#define PAGE_FRAME_SIZE (1 << PAGE_FRAME_SHIFT)
#define MAX_VAL 7 // max buddy frame size: 4KB * 2^6

typedef struct frame
{
	struct list_head listhead;
	int val;
} frame_t;

void *kmalloc(unsigned int size);
void kfree(void *ptr);
unsigned int get_memory_size();
void memory_init();
void *startup_malloc(unsigned long long int size);
int allocate_frame();
int allocate_frame_buddy(int begin_frame, int count, int val);
void *page_malloc(unsigned int size);
int page_free(void *frame);
frame_t *get_buddy(int val, frame_t *frame);
int page_insert(int val, frame_t *frame);
void print_frame();

#endif /* _MEMORY_H_ */
