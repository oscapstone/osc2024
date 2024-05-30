#ifndef __MM_TYPE_H__
#define __MM_TYPE_H__

#include <lib/stddef.h>

#define MAX_ORDER 16
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

#define PAGE_FREE 0
#define PAGE_ALLOCATED 1
#define PAGE_CACHE 2
#define PAGE_RESERVED 3

typedef uintptr_t phys_addr_t;  // 節點結構，代表自由區塊
typedef struct page {
    struct page *prev, *next;  // 指向同一 order 的下一個節點
    unsigned long order;
    uint8_t status;  // 0: free, 1: allocated
} page_t;

void buddy_init(phys_addr_t start, phys_addr_t end);
page_t *alloc_pages(unsigned long order);
void free_pages(page_t *page, unsigned long order);

phys_addr_t get_addr_by_page(page_t *page);
page_t *get_page_by_addr(phys_addr_t addr);

void memory_reserve(phys_addr_t, phys_addr_t);

#endif
