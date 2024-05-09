#ifndef PAGE_ALLOC_H
#define PAGE_ALLOC_H

#include "def.h"
#include "int.h"

void buddy_init(void);
void* alloc_pages(size_t request);
void free_pages(void* ptr);
void print_free_list(void);
void test_page_alloc(void);

#endif /* PAGE_ALLOC_H */
