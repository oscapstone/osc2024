#ifndef PAGE_ALLOC_H
#define PAGE_ALLOC_H

#include "def.h"
#include "int.h"
#include "list.h"

struct zone {
    uint64_t managed_pages;        // total number of pages
    struct free_area* free_areas;  // free lists for different power-of-2 pages.
};

struct free_area {
    struct list_head free_list;
    uint64_t nr_free;  // number of free block in the list
};

struct page {
    uint64_t flags;
    union {
        /* PAGE */
        struct {
            struct list_head list;  // to be put on free list
            uint64_t private;       // order
        };

        /* SLAB */
        struct {
            union {
                struct list_head slab_list;
                struct { /* partial page */
                    struct page* next;
                    int pages;     // nr of page left
                    int pobjects;  // approximate count
                };
            };

            struct kmem_cache* slab_cache;  // which slab cache is this slab in?
            void* freelist;                 // first free object

            union {
                uint64_t counters;
                struct {
                    unsigned inuse   : 16;
                    unsigned objects : 15;
                    unsigned frozen  : 1;
                };
            };
        };

        /* Compound page */
        struct {
            uint64_t compound_head;
            uint8_t compound_order;
        };
    };
};


void buddy_init(void);
size_t order_for_request(size_t request);
struct page* alloc_pages(size_t order);
void free_pages(struct page* page_ptr, size_t order);
void print_free_list(void);
void test_page_alloc(void);
struct page* get_page_from_ptr(void* ptr);

#endif /* PAGE_ALLOC_H */
