#ifndef __MM_H__
#define __MM_H__

#define PAGE_SHIFT                  (12)
#define MAX_ORDER                   (10)
// #define NR_PAGES                    (262144)
#define NR_PAGES                    (nr_pages)

#ifndef __ASSEMBLER__

#include "list.h"
#include "stdint.h"
#include "slab.h"

#define PG_USED                     (1) // page frame used.
#define PG_KERNEL                   (2) // page frame used by kernel.
#define PG_MMIO                     (3) // page frame used by MMIO.
#define PG_AVAIL                    (4) // page frame is free and not in buddy system. e.g. order 2 page fram [0, 1, 2, 3], flags  = [PG_buddy, PG_AVAIL, PG_AVAIL, PG_AVAIL]
#define PG_RESERVED                 (0x80) // page frame is reserved.

#define PG_slab                     (0x20) // page frame is used by slab allocator.
#define PG_buddy                    (0x40) // page frame is free and in buddy system.

struct page {
    unsigned long flags; // page flags: represent the page state, like compound, dirty, etc.
    union {
        struct {
            struct list_head buddy_list; // list_head for buddy system.
            unsigned long private; // used to indicate the order of the page in buddy system.
        };
        struct { /* Used for slab */
            struct list_head slab_list; // list_head for other slabs.
            struct kmem_cache *slab_cache; // point back to the `struct kmem_cache`
            void *freelist; // the address of the free list.

            union { // frozen and inuse are not implemented for now. Just use the counter to represent the number of objects
                unsigned long counters;
                struct {
                    unsigned inuse:16;
                    unsigned objects:15;
                    unsigned frozen:1;
                };
            };
        };
    };
};

struct free_area {
    /* At linux kernel, the free_list is an array for different MIGRATE_TYPE. We don't implement it for now. */
    struct list_head free_list;
    unsigned long nr_free; // number of free pages of this order
};

struct zone {
    unsigned long managed_pages; // number of pages managed by buddy system.
    // unsigned long spanned_pages; // number of pages in the zone. not used for now.

    struct free_area free_area[MAX_ORDER]; // the key data structure for buddy system

    /* active_list and inactive_list is for page reclaim, not used for now */
    // struct list_head active_list;
    // struct list_head inactive_list;
};

extern struct page *mem_map; // mem_map points to pages

#define pfn_to_page(pfn) (mem_map + pfn)
#define page_to_pfn(page) ((unsigned long)((page) - mem_map))

#define pfn_to_phys(pfn) ((unsigned long)(pfn << PAGE_SHIFT))
#define phys_to_pfn(phys) ((unsigned long)phys >> PAGE_SHIFT)

#define page_to_phys(page) (pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys) (pfn_to_page(phys_to_pfn(phys)))


/* Init buddy system and slab allocator. */
void mm_init(void);

/* Allocate page from buddy system. */
struct page *__alloc_pages(unsigned int order);

/* Free a page and merge it into buddy system. Without lock protection for now. */
void free_one_page(struct page *page, unsigned long pfn, unsigned int order);

/* Print out the buddy system information */
void get_buddy_info(void);

/* Reserve memory to mem_block. */
void reserve_mem(unsigned long start, unsigned long end);

/* Kernel memory allocate, return physical address. */
void *kmalloc(unsigned long size);
void kfree(void *obj);

#endif // __ASSEMBLER__
#endif // __MM_H__