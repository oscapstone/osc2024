#include "mm.h"
#include "uart.h"

#define MAX_ORDER 5
#define NUM_PAGES 0x3C000
#define PAGE_SIZE 0x1000

static struct page mem_map[32];
static struct page *free_area[MAX_ORDER + 1];

void free_list_push(struct page **list_head, struct page *page,
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

struct page *free_list_pop(struct page **list_head)
{
    if (*list_head == 0)
        return 0;

    struct page *page = *list_head;
    *list_head = page->next;
    page->used = 1;
    return page;
}

void free_list_remove(struct page **list_head, struct page *page)
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
    for (int i = MAX_ORDER; i >= 0; i--) {
        uart_puts("[");
        uart_hex(i);
        uart_puts("]: ");
        struct page *current = free_area[i];
        while (current != 0) {
            uart_hex(current - mem_map);
            uart_puts("-");
            current = current->next;
        }
        uart_puts("\n");
    }
    uart_puts("\n");
}

static struct page *get_buddy(struct page *page, unsigned int order)
{
    unsigned int buddy_pfn = (unsigned int)(page - mem_map) ^ (1 << order);
    return &mem_map[buddy_pfn];
}

void mem_init()
{
    for (int i = 0; i < 16; i++) {
        mem_map[i].order = 0;
        mem_map[i].prev = 0;
        mem_map[i].next = 0;
    }
    free_list_push(&free_area[MAX_ORDER], mem_map, MAX_ORDER);
}

struct page *alloc_pages(unsigned int order)
{
    for (int i = order; i <= MAX_ORDER; i++) {
        if (free_area[i] == 0)
            continue;

        struct page *page = free_list_pop(&free_area[i]);

        while (i > order) {
            i--;
            struct page *buddy = get_buddy(page, i);
            free_list_push(&free_area[i], buddy, i);
        }
        return page;
    }
    return 0;
}

void free_pages(struct page *page, unsigned int order)
{
    struct page *current = page;
    while (order < MAX_ORDER) {
        struct page *buddy = get_buddy(current, order);
        if (buddy->order != order || buddy->used == 1)
            break;

        free_list_remove(&free_area[order], buddy);
        if (current > buddy)
            current = buddy;

        order++;
    }
    free_list_push(&free_area[order], current, order);
}