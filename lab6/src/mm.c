#include "mm.h"
#include "alloc.h"
#include "devtree.h"
#include "string.h"
#include "uart.h"
#include "utils.h"
#include "vm.h"

extern char *__bss_end;
extern void *DTB_BASE;

#define BUDDY_MAX_ORDER 10
#define CACHE_MAX_ORDER 6

#define NUM_PAGES 0x3C000
#define PAGE_SIZE 0x1000

static struct page *mem_map;
static struct page *free_area[BUDDY_MAX_ORDER + 1];
static struct object *kmem_cache[CACHE_MAX_ORDER + 1];

/* Buddy Allocator */

static void free_list_push(struct page **list_head, struct page *page,
                           unsigned int order)
{
    page->order = order;
    page->used = 0;
    page->prev = 0;
    page->next = 0;

    if (*list_head == 0) {
        *list_head = page;
        return;
    }

    page->next = *list_head;
    (*list_head)->prev = page;
    *list_head = page;
}

static struct page *free_list_pop(struct page **list_head)
{
    if (*list_head == 0)
        return 0;

    struct page *page = *list_head;
    *list_head = page->next;
    page->used = 1;
    return page;
}

static void free_list_remove(struct page **list_head, struct page *page)
{
    if (page->prev != 0)
        page->prev->next = page->next;
    if (page->next != 0)
        page->next->prev = page->prev;
    if (page == *list_head)
        *list_head = page->next;
}

void free_list_display()
{
    for (int i = BUDDY_MAX_ORDER; i >= 0; i--) {
        uart_puts("[BUDDY] free_area[");
        uart_hex(i);
        uart_puts("] = ");
        struct page *page = free_area[i];
        int count = 0;
        while (page != 0) {
            // uart_hex((unsigned long)(page - mem_map));
            // uart_puts("-");
            page = page->next;
            count++;
        }
        uart_hex(count);
        uart_puts("\n");
    }
}

static struct page *get_buddy(struct page *page, unsigned int order)
{
    unsigned int buddy_pfn = (unsigned int)(page - mem_map) ^ (1 << order);
    return &mem_map[buddy_pfn];
}

struct page *alloc_pages(unsigned int order)
{
    // uart_puts("[BUDDY] (Allocate ");
    // uart_hex(1 << order);
    // uart_puts(" pages)\n");

    for (int i = order; i <= BUDDY_MAX_ORDER; i++) {
        if (free_area[i] == 0)
            continue;

        struct page *page = free_list_pop(&free_area[i]);
        page->order = order;

        while (i > order) {
            i--;
            struct page *buddy = get_buddy(page, i);
            free_list_push(&free_area[i], buddy, i);

            // Print information
            // unsigned int pfn = page - mem_map;
            // unsigned int buddy_pfn = buddy - mem_map;
            // uart_puts("[BUDDY] Split ");
            // uart_hex(pfn);
            // uart_puts("-");
            // uart_hex((pfn + (1 << (i + 1))));
            // uart_puts(" into ");
            // uart_hex(pfn);
            // uart_puts("-");
            // uart_hex((pfn + (1 << i)));
            // uart_puts(" and ");
            // uart_hex(buddy_pfn);
            // uart_puts("-");
            // uart_hex((buddy_pfn + (1 << i)));
            // uart_puts("\n");
        }

        // uart_puts("[BUDDY] Alloc ");
        // uart_hex((unsigned int)(page - mem_map));
        // uart_puts("-");
        // uart_hex((unsigned int)(page - mem_map + (1 << order)));
        // uart_puts("\n");

        return page;
    }
    return 0;
}

void free_pages(struct page *page, unsigned int order)
{
    // uart_puts("[BUDDY] (Free ");
    // uart_hex(1 << order);
    // uart_puts(" pages)\n");

    struct page *current = page;
    while (order < BUDDY_MAX_ORDER) {
        struct page *buddy = get_buddy(current, order);
        if (buddy->order != order || buddy->used == 1)
            break;

        free_list_remove(&free_area[order], buddy);

        if (current > buddy) {
            struct page *tmp = current;
            current = buddy;
            buddy = tmp;
        }

        order++;

        // Print information
        // unsigned int pfn = current - mem_map;
        // unsigned int buddy_pfn = buddy - mem_map;
        // uart_puts("[BUDDY] Merge ");
        // uart_hex(pfn);
        // uart_puts("-");
        // uart_hex((pfn + (1 << (order - 1))));
        // uart_puts(" and ");
        // uart_hex(buddy_pfn);
        // uart_puts("-");
        // uart_hex((buddy_pfn + (1 << (order - 1))));
        // uart_puts(" into ");
        // uart_hex(pfn);
        // uart_puts("-");
        // uart_hex((pfn + (1 << order)));
        // uart_puts("\n");
    }
    free_list_push(&free_area[order], current, order);
}

/* Cache Allocator */

static void cache_list_push(struct object **list_head, struct object *object,
                            unsigned int order)
{
    object->order = order;
    object->next = *list_head;
    *list_head = object;
}

static struct object *cache_list_pop(struct object **list_head)
{
    if (*list_head == 0)
        return 0;

    struct object *object = *list_head;
    *list_head = object->next;
    return object;
}

void *kmem_cache_alloc(unsigned int order)
{
    // uart_puts("[CACHE] (Allocate ");
    // uart_hex(32 << order);
    // uart_puts(" bytes)\n");

    if (kmem_cache[order] == 0) {
        struct page *page = alloc_pages(0);
        page->cache = order;
        unsigned long page_addr = PHYS_TO_VIRT((page - mem_map) * PAGE_SIZE);
        unsigned int cache_size = 32 << order;
        for (int i = 0; i < PAGE_SIZE; i += cache_size) {
            struct object *obj = (struct object *)(uintptr_t)(page_addr + i);
            cache_list_push(&kmem_cache[order], obj, order);
        }
    }
    return cache_list_pop(&kmem_cache[order]);
}

void kmem_cache_free(void *ptr, unsigned int index)
{
    // uart_puts("[CACHE] (Free ");
    // uart_hex(32 << index);
    // uart_puts(" bytes)\n");

    cache_list_push(&kmem_cache[index], ptr, index);
}

/* Dynamic Memory Allocator */

void *kmalloc(unsigned int size)
{
    if (size == 0)
        return 0;

    // uart_puts("[ALLOC] (Allocate ");
    // uart_hex(size);
    // uart_puts(" bytes)\n");

    if (size > (32 << CACHE_MAX_ORDER)) {
        // Buddy Allocator
        int order = 0;
        while ((PAGE_SIZE << order) < size)
            order++;
        struct page *page = alloc_pages(order);
        return (void *)PHYS_TO_VIRT((page - mem_map) * PAGE_SIZE);
    } else {
        // Cache Allocator
        int power = 0;
        while ((1 << power) < size)
            power++;
        int order = (power > 5) ? power - 5 : 0;
        return kmem_cache_alloc(order);
    }
}

void kfree(void *ptr)
{
    // If the pointer is page-aligned and its corresponding
    // page is not divided into cache objects, then free the
    // page directly. Otherwise, free the cache object.
    if ((intptr_t)VIRT_TO_PHYS(ptr) % PAGE_SIZE == 0) {
        struct page *page = &mem_map[(intptr_t)VIRT_TO_PHYS(ptr) / PAGE_SIZE];
        if (page->cache == -1) {
            free_pages(page, page->order);
            return;
        }
    }
    // TODO: Check if the pointer is a valid cache object
    struct object *object = ptr;
    kmem_cache_free(object, object->order);
}

void mem_init()
{
    // Initialize the buddy allocator
    mem_map = simple_malloc(sizeof(struct page) * NUM_PAGES);
    for (int i = NUM_PAGES - 1; i >= 0; i--) {
        mem_map[i].order = 0;
        mem_map[i].used = 0;
        mem_map[i].cache = -1;
        mem_map[i].prev = 0;
        mem_map[i].next = 0;
        if (i % (1 << BUDDY_MAX_ORDER) == 0)
            free_list_push(&free_area[BUDDY_MAX_ORDER], &mem_map[i],
                           BUDDY_MAX_ORDER);
    }

    // Fetch the memory information from the device tree
    uint64_t devtree_start = (uint64_t)DTB_BASE, devtree_end;
    uint64_t initrd_start, initrd_end;
    uintptr_t dtb_ptr = (uintptr_t)DTB_BASE;
    struct fdt_header *header = (struct fdt_header *)dtb_ptr;
    devtree_end = devtree_start + be2le(&header->totalsize);
    uintptr_t structure = (uintptr_t)header + be2le(&header->off_dt_struct);
    uintptr_t strings = (uintptr_t)header + be2le(&header->off_dt_strings);
    uint32_t structure_size = be2le(&header->size_dt_struct);
    uintptr_t ptr = structure;
    while (ptr < structure + structure_size) {
        uint32_t token = be2le((char *)ptr);
        ptr += 4;
        switch (token) {
        case FDT_BEGIN_NODE:
            ptr += align4(strlen((char *)ptr) + 1);
            break;
        case FDT_END_NODE:
            break;
        case FDT_PROP:
            uint32_t len = be2le((char *)ptr);
            ptr += 4;
            uint32_t nameoff = be2le((char *)ptr);
            ptr += 4;
            if (!strcmp((char *)(strings + nameoff), "linux,initrd-start"))
                initrd_start = (uint64_t)be2le((void *)ptr);
            if (!strcmp((char *)(strings + nameoff), "linux,initrd-end"))
                initrd_end = (uint64_t)be2le((void *)ptr);
            ptr += align4(len);
            break;
        case FDT_NOP:
            break;
        case FDT_END:
            break;
        }
    }

    // Reserve memory
    //   Spin tables for multicore boot
    //   Kernel image
    //   Simple allocator
    //   Devicetree
    //   Initramfs
    memory_reserve(0x0, VIRT_TO_PHYS((unsigned long)&__bss_end + 0x800000));
    memory_reserve(devtree_start, devtree_end);
    memory_reserve(initrd_start, initrd_end);
}

void memory_reserve(uint64_t start, uint64_t end)
{
    // Round the start and end addresses to the page boundary
    start = start & ~(PAGE_SIZE - 1);
    end = (end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    end--;

    uart_puts("[INFO] Reserve memory from ");
    uart_hex(start);
    uart_puts(" to ");
    uart_hex(end);
    uart_puts("\n");

    for (int order = BUDDY_MAX_ORDER; order >= 0; order--) {
        struct page *current = free_area[order];
        while (current != 0) {
            struct page *next = current->next;
            // Note: page_start and page_end are the physical addresses
            uint64_t page_start = (current - mem_map) * PAGE_SIZE;
            uint64_t page_end = page_start + (PAGE_SIZE << order) - 1;
            if (page_start >= start && page_end <= end) {
                // The page is within the reserved memory range
                // Remove the page from the free list
                current->used = 1;
                free_list_remove(&free_area[order], current);
            } else if (start > page_end || end < page_start) {
                // The page is outside the reserved memory range
            } else {
                // The page overlaps with the reserved memory range
                // Split the page into two parts
                struct page *half = get_buddy(current, order - 1);
                free_list_remove(&free_area[order], current);
                free_list_push(&free_area[order - 1], half, order - 1);
                free_list_push(&free_area[order - 1], current, order - 1);
            }
            current = next;
        }
    }
}