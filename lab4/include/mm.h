#ifndef MM_H
#define MM_H

struct page {
    unsigned int order;
    unsigned int used;
    struct page *prev;
    struct page *next;
};

void mem_init();
void memory_reserve(void *start, void *end);

struct page *alloc_pages(unsigned int order);
void free_pages(struct page *page, unsigned int order);

void *kmalloc(unsigned int size);
void kfree(void *ptr);

#endif // MM_H