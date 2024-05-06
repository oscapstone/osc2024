#include <kernel/io.h>
#include <kernel/memory.h>

#include "mm.h"

static page_t *mem_map;
static page_t *free_areas[MAX_ORDER + 1];

static inline void __push_page(page_t **head, page_t *page) {
    if (*head == NULL) {
        *head = page;
        return;
    }

    page->next = *head;
    (*head)->prev = page;
    *head = page;
}

static inline page_t *__pop_page(page_t **head) {
    if (*head == NULL) {
        return NULL;
    }
    page_t *page = *head;
    page->next = NULL;

    *head = (*head)->next;
    if (*head) {  // unlink the head
        (*head)->prev = NULL;
    }
    return page;
}

static inline void __remove_page(page_t **head, page_t *page) {
    if (page->prev) {
        page->prev->next = page->next;
    } else {
        *head = page->next;
    }

    if (page->next) {
        page->next->prev = page->prev;
    }
}

void buddy_init(phys_addr_t start, phys_addr_t end) {
    uintptr_t buddy_size = (uintptr_t)end - (uintptr_t)start;

    print_string("Buddy size: ");
    print_d(buddy_size);
    print_string("\nFrom ");
    print_h((uint64_t)start);
    print_string(" to ");
    print_h((uint64_t)end);
    print_string("\n");

    // split the memory into max order pages and add them to free areas
    for (int i = 0; i <= MAX_ORDER; i++) {
        free_areas[i] = NULL;
    }

    int num_of_pages = buddy_size / PAGE_SIZE;

    print_string("Number of pages: ");
    print_d(num_of_pages);
    print_string("\n");

    mem_map = simple_malloc(num_of_pages * sizeof(page_t));
    for (int i = num_of_pages - 1; i >= 0; i--) {
        mem_map[i].order = MAX_ORDER;
        mem_map[i].status = PAGE_FREE;
        mem_map[i].prev = NULL;
        mem_map[i].next = NULL;

        if (i % (1 << MAX_ORDER) ==
            0) {  // push the chunk of page to the free area
            __push_page(&free_areas[MAX_ORDER], &mem_map[i]);
        }
    }

    print_free_areas();
}

page_t *alloc_pages(unsigned long order) {
    unsigned long cur_order;
    for (cur_order = order; cur_order <= MAX_ORDER; cur_order++) {
        if (free_areas[cur_order]) {
            page_t *page = __pop_page(&free_areas[cur_order]);

            // split the page into smaller pages
            while (cur_order > order) {
                cur_order--;
                uint32_t buddy_pfn = (page - mem_map) ^ (1 << cur_order);
                page_t *buddy = &mem_map[buddy_pfn];
                buddy->order = cur_order;
                __push_page(&free_areas[cur_order], buddy);
            }

            page->order = order;
            page->status = PAGE_ALLOCATED;
            return page;
        }
    }
    return NULL;  // 沒有可用記憶體
}

void free_pages(page_t *page, unsigned long order) {
    int cur_order = order;
    for (; cur_order < MAX_ORDER; cur_order++) {
        uint32_t buddy_pfn = (page - mem_map) ^ (1 << cur_order);
        page_t *buddy = &mem_map[buddy_pfn];

        print_string("\ni: ");
        print_d(cur_order);
        print_string("----------\n");
        print_string("Page: ");
        print_h((uint64_t)get_addr_by_page(page));
        print_string("\nBuddy: ");
        print_h((uint64_t)get_addr_by_page(buddy));
        print_string("\nbuddy_order: ");
        print_d(buddy->order);
        print_string("\nbuddy_status: ");
        print_d(buddy->status);

        if (buddy->order != cur_order || buddy->status != PAGE_FREE) {
            break;
        }

        __remove_page(&free_areas[cur_order], buddy);
        page = (page < buddy)
                   ? page
                   : buddy;  // update the page pointer to the lower address
    }

    page->status = PAGE_FREE;
    page->order = cur_order;
    print_string("\nPush page: ");
    print_h((uint64_t)get_addr_by_page(page));
    print_string("\nTo order ");
    print_d(cur_order);
    print_string("\n");
    __push_page(&free_areas[cur_order], page);
}

void print_free_areas() {
    for (int i = 0; i <= MAX_ORDER; i++) {
        page_t *page = free_areas[i];
        print_string("Order ");
        print_d(i);
        print_string(": ");
        int count = 0;
        while (page) {
            print_h((uint64_t)get_addr_by_page(page));
            print_string(" -> ");
            page = page->next;

            // if (count++ > 100) {
                // break;
            // }
        }
        print_string("NULL\n");
    }
}

phys_addr_t get_addr_by_page(page_t *page) {
    // print_string("Get addr by page: ");
    // print_h((uint64_t)(((page - mem_map) << PAGE_SHIFT) + (void *)start));
    // print_string("\n");
    return (phys_addr_t)((page - mem_map) << PAGE_SHIFT);
}

page_t *get_page_by_addr(phys_addr_t addr) {
    // print_string("Get page by addr: ");
    // print_h((uint64_t)&mem_map[(addr - start) >> PAGE_SHIFT]);
    return &mem_map[(uint64_t)(void *)addr >> PAGE_SHIFT];
}

void memory_reserve(phys_addr_t start, phys_addr_t end) {
    print_string("Reserve memory: ");
    print_h((uint64_t)start);
    print_string(" - ");
    print_h((uint64_t)end);
    print_string("\n");
    
    for(int cur_order = MAX_ORDER; cur_order >= 0; cur_order--) {
        page_t *page = free_areas[cur_order];
        while(page != NULL) {
            page_t *next_page = page->next;

            phys_addr_t page_addr = get_addr_by_page(page);
            phys_addr_t page_end = (void*)page_addr + (1 << (cur_order + PAGE_SHIFT)) - 1;

            if(cur_order == 0) {
                if(!(page_end < start || page_addr > end)) {
                    print_string("\nPage partially reserved\n");
                    page->status = PAGE_RESERVED;
                    __remove_page(&free_areas[cur_order], page);
                }
            } else if(page_addr >= start && page_end <= end) {
                print_string("\nPage fully reserved\n");
                print_h((uint64_t)page_addr);
                print_string(" - ");
                print_h((uint64_t)page_end);
                print_string("\n");

                page->status = PAGE_RESERVED;
                __remove_page(&free_areas[cur_order], page);
            } else if(start > page_end || end < page_addr) { // page out of range
                // print_string("\nPage out of range\n");
            } else { // split the overlapping page using buddy
                // print_string("\nFind buddy\n");
                // print_string("\nPage: ");
                // print_h((uint64_t)page_addr);
                // print_string("\n");
                // print_string("\nCur order: ");
                // print_d(cur_order);
                // print_string("\n");

                uint32_t buddy_pfn = (page - mem_map) ^ (1 << (cur_order - 1));
                page_t *buddy = &mem_map[buddy_pfn];

                print_string("\nSplit page: ");
                print_h((uint64_t)page_addr);
                print_string(", ");
                print_h((uint64_t)get_addr_by_page(buddy));
                print_string(", ");
                print_h((uint64_t)buddy_pfn);
                print_string("\n");

                // remove the original page from the free area
                print_string("\nRemove page ");
                print_h((uint64_t)get_addr_by_page(page));
                print_string(" from order ");
                print_d(cur_order);
                print_string("\n");
                __remove_page(&free_areas[cur_order], page);
                buddy->order = cur_order - 1;
                page->order = cur_order - 1;
                page->prev = NULL;
                // push split page to the free area
                __push_page(&free_areas[cur_order - 1], buddy);
                __push_page(&free_areas[cur_order - 1], page);
            }

            // print_string("\nNext page\n");
            // print_h((uint64_t)get_addr_by_page(next_page));

            page = next_page;
        }
    }
}
