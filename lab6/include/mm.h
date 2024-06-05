#ifndef MM_H
#define MM_H

#include <stdint.h>

#define PAGE_SIZE 0x1000

struct page {
    unsigned int order;
    unsigned int used;
    unsigned int cache;
    struct page *prev;
    struct page *next;
};

struct object {
    unsigned int order;
    struct object *next;
};

struct page *alloc_pages(unsigned int order);
void free_pages(struct page *page, unsigned int order);

void *kmem_cache_alloc(unsigned int index);
void kmem_cache_free(void *ptr, unsigned int index);

void *kmalloc(unsigned int size);
void kfree(void *ptr);

void mem_init();
void memory_reserve(uint64_t start, uint64_t end);
void free_list_display();

#endif // MM_H
