#ifndef MEMORY_H
#define MEMORY_H

#include "def.h"
#include "int.h"
#include "mm.h"

extern void *mem_align(void *, uint64_t);
int mem_init(uintptr_t dtb_ptr);
void mem_reserve(uintptr_t start, uintptr_t end);
void *mem_alloc(uint64_t);
void mem_free(void *);

void buddy_init(void);
uint8_t *alloc_pages(size_t request);
void free_pages(uint8_t *ptr);

void print_free_list(void);
void test_page_alloc(void);

#endif /* MEMORY_H */
