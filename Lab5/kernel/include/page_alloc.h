#ifndef PAGE_ALLOC_H
#define PAGE_ALLOC_H

#include "def.h"
#include "gfp_types.h"
#include "int.h"
#include "list.h"

#define ALIGN(x, a)    (((x) + (a) - 1) & ~((a) - 1))
#define page_alignment 64

struct zone {
    uint64_t managed_pages;        // total number of pages
    struct free_area* free_areas;  // free lists for different power-of-2 pages.
};

struct free_area {
    struct list_head free_list;
    uint64_t nr_free;  // number of free block in the list
};

// 48-bytes
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
} __attribute__((aligned(page_alignment)));


void buddy_init(void);
struct page* alloc_pages(size_t order, gfp_t types);
void free_pages(struct page* page_ptr, size_t order);

void buddyinfo(void);
void pageinfo(void);

void test_page_alloc(void);

struct page* get_compound_head(struct page* page);
unsigned int get_compound_order(struct page* page);

void* PAGE_ALIGN_DOWN(void* x);
void* PAGE_ALIGN_UP(void* x);

void* pfn_to_phys(size_t pfn);
size_t phys_to_pfn(void* phys);

struct page* pfn_to_page(size_t pfn);
size_t page_to_pfn(struct page* page);

void* page_to_phys(struct page* page);
struct page* phys_to_page(void* phys);

size_t get_order(size_t size);

#endif /* PAGE_ALLOC_H */
