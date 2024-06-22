#ifndef MEM_UTILS_H
#define MEM_UTILS_H

#include <stdint.h>
#define NULL 0

/* for simple allocation or startup allocation */
char *mem_align(char *addr, unsigned int number);
void *malloc(unsigned int size);

/* Page frame allocator API or dynamic allocator API*/
void *show_heap_end(void);
void buddy_system_init(void);
void dynamic_allocator_init(void);
void *page_frame_allocate(uint32_t size);
void page_frame_free(char *addr);
void memory_reserve(char *start, char *end);
void show_memory_layout(void);
void *chunk_alloc(uint32_t size);
void chunk_free(char *addr);

#endif