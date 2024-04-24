/**
 * First, we need to initialize the memory map (pages[]). So we have to decide the memory layout. Some for kernel and some for MMIO.
 * Then we should implement the mapping between page structure and page frame number (pfn) in mm.h
*/

#include "mm.h"
#include "uart.h"
#include "slab.h"
#include "memblock.h"
#include "stddef.h"
#include "demo.h"

#ifndef MMIO_BASE
#define MMIO_BASE               (0x3F000000)
#endif // MMIO_BASE

#ifndef PAGE_SIZE
#define PAGE_SIZE               (1 << 12) // 4KB
#endif // PAGE_SIZE

#ifdef DEMO
int isDemo = 0;
#endif

/* page frame: to map the physical memory */
// static struct page pages[NR_PAGES] = {0}; // Not sure whether we should initialize it or not. (.bss problem)
struct page *mem_map = NULL;

unsigned long nr_pages = 0;

/* Though we don't have NUMA, use one zone to represent the whole memory space */
static struct zone zone = {0};

extern unsigned char _end; // the end of kernel image

/* Find the buddy of a given page frame number and order. */
static inline unsigned long
__find_buddy_pfn(unsigned long page_pfn, unsigned int order)
{
    return page_pfn ^ (1 << order);
}

/* This function returns the order of a free page in the buddy system. */
static inline unsigned long buddy_order(struct page *page)
{
    return page->private;
}

/* To check whether the page frame is at buddy system.*/
static inline bool PageBuddy(struct page *page)
{
    return page->flags == PG_buddy;
}

/* Check whether the page is free && is buddy. If buddy is page's buddy, return 1. Else 0 */
static inline bool page_is_buddy(struct page *page, struct page *buddy, unsigned int order)
{
    return (buddy->flags == PG_buddy && buddy->private == order);
}

/* Find the buddy of given page and validate it. */
static inline struct page *find_buddy_page_pfn(struct page *page, unsigned long pfn, unsigned int order, unsigned long *buddy_pfn)
{
    unsigned long __buddy_pfn = __find_buddy_pfn(pfn, order);
    struct page *buddy;
    
    buddy = page + (__buddy_pfn - pfn);
    if (buddy_pfn)
        *buddy_pfn = __buddy_pfn;
    
    if (page_is_buddy(page, buddy, order))
        return buddy;
    return NULL;
}

/* Remove the page from free_area. */
static inline void del_page_from_free_area(struct page *page, unsigned int order)
{
    list_del(&(page->buddy_list));
    zone.free_area[order].nr_free--;
    zone.managed_pages--;
}

/* Add the page into free_list of given order. */
static inline void add_to_free_area(struct page *page, unsigned int order)
{
    list_add_tail(&(page->buddy_list), &(zone.free_area[order].free_list));
    zone.free_area[order].nr_free++;
    zone.managed_pages++;
}

/* Setup page order. */
static inline void set_page_order(struct page *page, unsigned int order)
{
    page->private = order;
}

/* Setup page->flags as PG_buddy */
static inline void set_page_buddy(struct page *page)
{
    page->flags = PG_buddy;
}

/* Setup order and `PG_buddy` */
static inline void set_buddy_order(struct page *page, unsigned int order)
{
    set_page_order(page, order);
    set_page_buddy(page);
}

/* Freeing function for a buddy system allocator. */
static inline void __free_one_page(struct page *page, unsigned long pfn, unsigned int order)
{
    unsigned long buddy_pfn;
    unsigned long combined_pfn;
    struct page *buddy;

    while (order < MAX_ORDER - 1) {
        /* Find the buddy page and validate it. */
        buddy = find_buddy_page_pfn(page, pfn, order, &buddy_pfn);
        if (!buddy)
            break;
        /* Remove the buddy from the free list. */
        del_page_from_free_area(buddy, order);

#ifdef DEMO
        if (isDemo) {
            uart_puts("[Free one page] Page found buddy. page: ");
            uart_hex(pfn);
            uart_puts(" order: ");
            uart_hex(order);
            uart_puts(" buddy: ");
            uart_hex(buddy_pfn);
            uart_send('\n');
        }
#endif
        /* Compute the combined pfn (combine the page and buddy) */
        combined_pfn = buddy_pfn & pfn;
        page = page + (combined_pfn - pfn);
        pfn = combined_pfn;
        /* Continue merging to higher-order free_area until no more merging is possible. */
        order++;
    }
    /* Set page order and `PG_buddy`. */
    set_buddy_order(page, order);

#ifdef DEMO
    if (isDemo) {
        uart_puts("[Free one page] Insert page: ");
        uart_hex(pfn);
        uart_puts(" order: ");
        uart_hex(order);
        uart_puts(" into buddy system\n");
    }
#endif

    /* Insert the final block into zone->free_area[order]. */
    add_to_free_area(page, order);
}

/* Initialize the page frame structure and the information in zone. */
static inline void page_frame_init(void)
{
    struct memblock_region *rgn;
    unsigned int i;

    /* Setup the number of pages from memblock.current_limit. */
    nr_pages = (memblock.current_limit >> 12) + 1;

    /* Allocate memory from memblock. */
    mem_map = (struct page *) memblock_phys_alloc(sizeof(struct page) * NR_PAGES);

    /* Initialize page structure*/
    for_each_memblock_type(i, &memblock.reserved, rgn) {
        // printf("Reserved: %x - %x\n", rgn->base, rgn->base + rgn->size);
        for (unsigned long pfn = rgn->base >> 12; pfn < (rgn->base + rgn->size) >> 12; pfn++) {
            mem_map[pfn].flags = PG_RESERVED;
            INIT_LIST_HEAD(&(mem_map[pfn].buddy_list));
        }
    }

    for (i = 0; i < nr_pages; i++) {
        if (mem_map[i].flags == PG_RESERVED)
            continue;
        mem_map[i].flags = PG_AVAIL;
        INIT_LIST_HEAD(&(mem_map[i].buddy_list));
        __free_one_page(&(mem_map[i]), i, 0); // Treat each page as a free page and insert it into the free list.
    }
#ifdef DEMO
    isDemo = 1;
#endif
}

/* Init the buddy system list_head structure */
static inline void buddy_free_area_init(void)
{
    int i = 0;
    for (; i < MAX_ORDER; i++) {
        INIT_LIST_HEAD(&(zone.free_area[i].free_list));
        zone.free_area[i].nr_free = 0;
    }
}

/* Get a free page from buddy system */
static inline struct page *get_page_from_free_area(struct free_area *area)
{
    if (list_empty(&(area->free_list)))
        return NULL;
    return list_first_entry(&(area->free_list), struct page, buddy_list);
}

/* Split the larger page and put it into the free list. */
static inline void expand(struct page *page, unsigned int low_order, unsigned int high_order)
{
    unsigned long size = 1 << high_order; // indicate the size of the page, then we need to split it and put it into the free list.

    if (low_order == high_order)
        return;

#ifdef DEMO
    if (isDemo) {
        uart_puts("[expand] page: ");
        uart_hex(page_to_phys(page));
        uart_puts(" order: ");
        uart_hex(high_order);
        uart_puts(" ,target order: ");
        uart_hex(low_order);
        uart_puts(" , so split the page\n");
    }
#endif

    /**
     * e.g. page = [0, 1, 2, 3, 4, 5, 6, 7], then we need to split it into [0, 1, 2, 3] and [4, 5, 6, 7].
     * Then we need to put [4, 5, 6, 7] into the free list of order 2. => `page[4].flags = PG_buddy` and insert it into the free list.
    */
    while (high_order > low_order) {
        high_order--;
        size >>= 1;

        /* In linux kernel, there's a procedure named `set_page_guard()`. But I don't understand its purpose (seems like debug information), skip it for now.*/

#ifdef DEMO
        if (isDemo) {
            uart_puts("[expand] insert page: ");
            uart_hex(page_to_phys(&(page[size])));
            uart_puts(" order: ");
            uart_hex(high_order);
            uart_puts(" into buddy system\n");
        }
#endif

        /* Setup order and `PG_buddy` */
        set_buddy_order(&(page[size]), high_order);
        /* Insert page[size] into free_list of order */
        add_to_free_area(&(page[size]), high_order);
    }
}

/* Go through the free lists and remove the smallest available page from the freelists. */
static inline struct page *__rmqueue_smallest(unsigned int order)
{
    struct page *page = NULL;
    struct free_area *area;
    unsigned int current_order;

#ifdef DEMO
    if (isDemo) {
        uart_puts("[rmqueue smallest] order: ");
        uart_hex(order);
        uart_send('\n');
    }
#endif

    for (current_order = order; current_order < MAX_ORDER; current_order++) {
        area = &(zone.free_area[current_order]);
        page = get_page_from_free_area(area);
        if (!page)
            continue;
        
        /* Remove the page from the free list. */
        del_page_from_free_area(page, current_order);
        /* Expand the page if the order is higher than we need. */
        expand(page, order, current_order);

        /* Setup page order before return. */
        set_page_order(page, order);

        /* Setup page.flags to PG_USED */
        page->flags = PG_USED;
        return page;
    }
    return NULL;
}

/* Get a free page from buddy system. Without lock protection for now. */
static inline struct page *rmqueue(unsigned int order)
{
    struct page *page = NULL;
    /* Linux kernel use spin_lock to protect critical region (buddy system), but I didn't implement it for now. */
    page = __rmqueue_smallest(order);
    return page;
}

/**
 * Initialize page frame structures and buddy system.
 * Make sure the memblock allocator has been initialized before this function. */
void buddy_init(void)
{
    /* Init the buddy system: free_area list_head structure. */
    buddy_free_area_init();
    /* We need to initialize the page frame structure `pages[]`. Then put free pages into buddy system. */
    page_frame_init();
}

/* Init buddy system and slab allocator. */
void mm_init(void)
{
    memblock_init();
    buddy_init();
    slab_init();
}

struct page *__alloc_pages(unsigned int order)
{
    return rmqueue(order);
}

/* Free a page and merge it into buddy system. Without lock protection for now. */
void free_one_page(struct page *page, unsigned long pfn, unsigned int order)
{
    /* Just make sure the page is from buddy system. */
    if (page->flags != PG_USED) {
        printf("Error: Try to free a page that is not allocated from buddy system.\n");
        return;
    }
    /* Linux kernel use spin_lock to protect critical region (buddy system), but I didn't implement it for now. */
    __free_one_page(page, pfn, order);
}


/* Get the memory information. */
void get_buddy_info(void)
{
    // printf("\n=====================\nBuddy System Information\n-------------------\n");
    // for (int i = 0; i < MAX_ORDER; i++) {
    //     printf("Order %d free pages: %d\n", i, zone.free_area[i].nr_free);
    // }
    // printf("=====================\n\n");

    uart_puts("\n=====================\nBuddy System Information\n-------------------\n");
    for (int i = 0; i < MAX_ORDER; i++) {
        uart_puts("Order ");
        uart_hex(i);
        uart_puts(" free pages: ");
        uart_hex(zone.free_area[i].nr_free);
        uart_send('\n');
    }
    uart_puts("=====================\n\n");
}

/* Kernel memory allocate, return physical address. */
void *kmalloc(unsigned long size)
{
    unsigned int order = 0;

    if (size >= PAGE_SIZE) {
        /* compute the order of the size */
        while (size > PAGE_SIZE) {
            size >>= 1;
            order++;
        }
        
        return (void *) page_to_phys(__alloc_pages(order));
    } else {
        /* Use slab allocator to allocate memory. */
        return (void *) get_object(size);
    }
}

void kfree(void *obj)
{
    struct page *page = phys_to_page((unsigned long) obj);
    unsigned long pfn = page_to_pfn(page);
    unsigned int order = buddy_order(page);

    if (page->flags == PG_slab) {
        /* Free the object to slab cache. */
        free_object(obj);
    } else {
        /* Free the page to buddy system. */
        free_one_page(page, pfn, order);
    }
}