#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "u_list.h"

/* Lab2 */
void* s_allocator(unsigned int size);
void  s_free(void* ptr);

/* Lab4 */
#define BUDDY_MEMORY_BASE       0x0     // 
#define BUDDY_MEMORY_PAGE_COUNT 0x3C000 // let BUDDY_MEMORY use 0x0 ~ 0x3C000000 (SPEC)
#define PAGESIZE    0x1000     // 4KB
#define MAX_PAGES   0x10000    // 65536 (Entries), PAGESIZE * MAX_PAGES = 0x10000000 (SPEC)

#define PAGE_MAX_IDX 6
#define PAGE_FREE -1
#define PAGE_ALLOCATED -2

#define SLAB_NONE -1
#define SLAB_MAX_IDX 7  //16B, 32B, 64B, 128B, 256B, 512B, 1024B, 2048B

#define INVALID_ADDRESS -1

typedef struct page
{
    struct list_head listhead; // store freelist
    int order;                   // store order
    int used;
    int slab_order;
    unsigned int idx;           //在physical memory中第幾個page
} page_t;

void     init_memory();
page_t *split_buddy(page_t *page);
page_t *find_buddy(page_t *page);
int merge_buddy(page_t *page);

void print_page_info();
void print_slab_info();

//buddy system
void* page_malloc(unsigned int size);
void  page_free(void *ptr);
void get_new_slab(int order);
void* slab_malloc(unsigned int size);
void  slab_free(void* ptr);
int pow(int base, int exponent);

void* kmalloc(unsigned int size);
void  kfree(void *ptr);
void  memory_reserve(unsigned long long start, unsigned long long end);

void swap(int*p1 ,int *p2);

#endif /* _MEMORY_H_ */
