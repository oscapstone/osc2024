#include <kernel/device_tree.h>
#include <kernel/io.h>
#include <kernel/memory.h>

#include "mm.h"

#define KMEM_CACHE_MIN_ORDER 4  // 16
#define KMEM_CACHE_MAX_ORDER 8  // 256

typedef struct kmem_cache {
    struct kmem_cache *next;
    unsigned long order;
} kmem_cache_t;

static kmem_cache_t
    *kmalloc_caches[KMEM_CACHE_MAX_ORDER - KMEM_CACHE_MIN_ORDER];

uint32_t get_page_order_by_size(uint32_t size) {
    uint32_t order = 0;
    while (size > (1 << (PAGE_SHIFT + order))) {
        order++;
    }
    print_string("\nGet order: ");
    print_d(order);
    print_string("\n");
    return order;
}

uint32_t get_cache_order_by_size(uint32_t size) {
    uint32_t order = 0;
    while (size > (1 << (order + KMEM_CACHE_MIN_ORDER))) {
        order++;
    }
    print_string("\nGet cache order: ");
    print_d(order);
    print_string("\n");
    return order;
}

extern uint64_t __heap_start;
void init_kmalloc() {
    // buddy_init((phys_addr_t)0x10000000, (phys_addr_t)0x20000000);
    buddy_init((phys_addr_t)0x0, (phys_addr_t)0x3C000000);
    memory_reserve(0x0, (phys_addr_t)0x1000);  // Spin tables for multicore boot
                                               // (0x0000 - 0x1000)
    memory_reserve((phys_addr_t)0x80000,
                   (phys_addr_t)&__heap_start);  // Kernel code and data

    memory_reserve(
        (phys_addr_t)&__heap_start,
        (phys_addr_t)((void *)&__heap_start + 0x100000));  // Startup allocator

    print_free_areas();

    memory_reserve((phys_addr_t)fdt_get_initrd_start(),
                   (phys_addr_t)fdt_get_initrd_end());  // Initramfs

    print_free_areas();
}

void print_kmalloc_caches() {
    for (int i = 0; i < KMEM_CACHE_MAX_ORDER - KMEM_CACHE_MIN_ORDER; i++) {
        kmem_cache_t *cache = kmalloc_caches[i];
        print_string("Cache Order ");
        print_d(i);
        print_string(": ");
        while (cache != NULL) {
            print_h((uint64_t)cache);
            print_string(" -> ");
            cache = cache->next;
        }
        print_string("NULL\n");
    }
}

void *kmem_cache_alloc(uint32_t order) {
    if (order > KMEM_CACHE_MAX_ORDER - KMEM_CACHE_MIN_ORDER) {
        return NULL;
    }
    if (kmalloc_caches[order] == NULL) {
        page_t *page = alloc_pages(0);

        if (page == NULL) {
            return NULL;
        }

        page->status = PAGE_CACHE;  // mark as cache page
        uint32_t cache_size = 1 << (order + KMEM_CACHE_MIN_ORDER);
        for (int i = 0; i < PAGE_SIZE; i += cache_size) {
            print_string("\n");
            print_d(i);
            kmem_cache_t *cache = ((void *)get_addr_by_page(page) + i);
            print_string("\ncache_addr: ");
            print_h((uint64_t)cache);
            print_string("\n");
            print_string(" .a ");
            cache->order = order;
            print_string(" .b ");
            cache->next = kmalloc_caches[order];
            print_string(" .c ");
            kmalloc_caches[order] = cache;
            print_string(" .d ");
        }
    }
    kmem_cache_t *cache = kmalloc_caches[order];
    kmalloc_caches[order] = cache->next;

    print_kmalloc_caches();

    return cache;
}

void kmem_cache_free(void *ptr, uint32_t order) {
    if (order > KMEM_CACHE_MAX_ORDER - KMEM_CACHE_MIN_ORDER) {
        return;
    }
    // push cache back to the list
    kmem_cache_t *cache = (kmem_cache_t *)ptr;
    cache->next = kmalloc_caches[order];
    kmalloc_caches[order] = cache;

    print_kmalloc_caches();
}

void *kmalloc(uint32_t size) {
    uint32_t order = get_page_order_by_size(size);
    if (order > MAX_ORDER) {
        return NULL;
    }

    if (order == 0) {
        if (size > (1 << KMEM_CACHE_MAX_ORDER)) {
            return (void *)get_addr_by_page(alloc_pages(order));
        }

        uint32_t cache_order = get_cache_order_by_size(size);
        return kmem_cache_alloc(cache_order);
    }

    return (void *)get_addr_by_page(alloc_pages(order));
}

void kfree(void *ptr) {
    if ((uintptr_t)ptr % PAGE_SIZE == 0) {
        page_t *page = get_page_by_addr((phys_addr_t)ptr);
        free_pages(page, page->order);
        return;
    }
    kmem_cache_t *cache = (kmem_cache_t *)ptr;
    kmem_cache_free(cache, cache->order);
}
