#ifndef _MEMORY_H_
#define _MEMORY_H_

typedef struct frame frame_t;

void *s_allocator(unsigned int size);

void init_allocator();
void *page_malloc(unsigned int size);
frame_t *release_redundant(frame_t *frame);
frame_t *get_buddy_frame(frame_t *frame);
void page_free(void *addr);
int coalesce(frame_t *frame);

void page2chunk(int order);
void *chunk_malloc(unsigned int size);
void chunk_free(void *addr);

void dump_page_info();
void dump_chunk_info();
void *kmalloc(unsigned int size);
void kfree(void *addr);

void memory_reserve(unsigned long long start, unsigned long long end);
void page_test();

#endif