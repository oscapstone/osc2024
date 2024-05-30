#include "memory.h"
#include "list.h"
#include "malloc.h"
#include "uart.h"

// #define BUDDY_MEMORY_START 0x10000000
// #define MAX_PAGES 0x10000 // PAGE_SIZE * MAX_PAGES = 0x20000000 - 0x10000000 = 0x10000000
#define BUDDY_MEMORY_START 0x0
#define MAX_PAGES 0x3C000 // PAGE_SIZE * MAX_PAGES = 0x3C000000 - 0x0 = 0x3C000000

#define PAGE_SIZE 0x1000 // 4KB
#define CHUNK_SIZE 0x20  // 32B

#define FRAME_FREE 0
#define FRAME_ALLOCATED 1
#define FRAME_MAX_ORDER 6

#define CHUNK_NONE -1
#define CHUNK_MAX_ORDER 6

typedef struct frame {
    int order;        // 0~6
    int used;         // 0: free, 1: used
    int chunk_order;  // -1~6
    unsigned int idx; // index in the buddy block list
    struct list_head list;
} frame_t;

extern char __start;
extern char __end;
extern char __heap_start;
extern char *cpio_start;
extern char *cpio_end;

static char *__heap_top = &__heap_start;
static frame_t *frame_pool;
static struct list_head buddy[FRAME_MAX_ORDER + 1];
static struct list_head chunk[CHUNK_MAX_ORDER + 1];

int mem_print = 0;

void *s_allocator(unsigned int size)
{
    char *r = __heap_top + 0x10;
    size += 0x10 - (size % 0x10);
    *(unsigned int *)(r - 0x8) = size;
    __heap_top += size;
    return r;
}

void init_allocator()
{
    // uart_hex(&__heap_start);
    // Initialize the frame pool from 0x10000000 to 0x20000000
    frame_pool = s_allocator(sizeof(frame_t) * MAX_PAGES);
    // frame_pool = (frame_t *)BUDDY_MEMORY_START;
    // uart_hex(&frame_pool[MAX_PAGES - 1]);

    for (int i = 0; i < MAX_PAGES; i++) {
        if (i % (1 << FRAME_MAX_ORDER) == 0) {
            frame_pool[i].order = FRAME_MAX_ORDER;
            frame_pool[i].used = FRAME_FREE;
        }
    }

    // Initialize the buddy list
    for (int i = 0; i <= FRAME_MAX_ORDER; i++) {
        INIT_LIST_HEAD(&buddy[i]);
    }

    // Initialize the chunk list
    for (int i = 0; i <= CHUNK_MAX_ORDER; i++) {
        INIT_LIST_HEAD(&chunk[i]);
    }

    // merge the buddy blocks
    for (int i = 0; i < MAX_PAGES; i++) {
        INIT_LIST_HEAD(&frame_pool[i].list);
        frame_pool[i].idx = i;
        frame_pool[i].chunk_order = CHUNK_NONE;

        if (i % (1 << FRAME_MAX_ORDER) == 0) {
            list_add_tail(&frame_pool[i].list, &buddy[FRAME_MAX_ORDER]);
        }
    }

    // Reserve memory for dtb, kernel image, and cpio
    memory_reserve(0, 4096);
    find_reserved_memory();
    memory_reserve((unsigned long long)&__start, (unsigned long long)&__end);
    memory_reserve((unsigned long long)cpio_start, (unsigned long long)cpio_end);
}

void *page_malloc(unsigned int size)
{
    if (mem_print) {
        uart_printf("\t[+] Allocate page size : %d (0x%x)\r\n", size, size);
        uart_printf("\t    Before\r\n");
        dump_page_info();
    }

    // Find the smallest order that can hold the size (4KB * 2^order)
    int val;
    for (int i = 0; i <= FRAME_MAX_ORDER; i++) {
        if (size <= (PAGE_SIZE << i)) {
            val = i;
            break;
        }

        if (i == FRAME_MAX_ORDER) {
            return NULL;
        }
    }

    int target_val = val;
    while (list_empty(&buddy[target_val]) && target_val <= FRAME_MAX_ORDER) {
        target_val++;
    }
    if (target_val > FRAME_MAX_ORDER) {
        return NULL;
    }

    frame_t *frame = list_first_entry(&buddy[target_val], frame_t, list);
    list_del(&frame->list); // remove from the buddy list
    for (int i = target_val; i > val; i--) {
        release_redundant(frame);
    }

    frame->used = FRAME_ALLOCATED;
    if (mem_print) {
        uart_printf("\t    physical address : 0x%x\n", BUDDY_MEMORY_START + (PAGE_SIZE * (frame->idx)));
        uart_printf("\t    After\r\n");
        dump_page_info();
    }

    return (void *)(long)(BUDDY_MEMORY_START + (PAGE_SIZE * (frame->idx)));
}

frame_t *release_redundant(frame_t *frame)
{
    frame->order--;
    frame_t *buddy_f = get_buddy_frame(frame);
    buddy_f->order = frame->order;
    list_add_tail(&buddy_f->list, &buddy[frame->order]);
    return frame;
}

frame_t *get_buddy_frame(frame_t *frame)
{
    // Find the buddy block of the frame
    unsigned int buddy_idx = frame->idx ^ (1 << frame->order);
    return &frame_pool[buddy_idx];
}

void page_free(void *addr)
{
    if (mem_print) {
        uart_printf("\t[+] Free page address : 0x%x\r\n", addr);
        uart_printf("\t    Before\r\n");
        dump_page_info();
    }

    frame_t *frame = &frame_pool[((unsigned long long)addr - BUDDY_MEMORY_START) >> 12];
    frame->used = FRAME_FREE;

    while (coalesce(frame))
        ;

    list_add_tail(&frame->list, &buddy[frame->order]);

    if (mem_print) {
        uart_printf("\t    After\r\n");
        dump_page_info();
    }
}

int coalesce(frame_t *frame)
{
    frame_t *buddy_f = get_buddy_frame(frame);
    if (frame->order == FRAME_MAX_ORDER || buddy_f->order != frame->order || buddy_f->used == FRAME_ALLOCATED)
        return 0;

    list_del(&buddy_f->list);
    if (buddy_f < frame)
        frame = buddy_f;
    frame->order++;

    if (mem_print)
        uart_printf("\t    merge 0x%x, 0x%x -> order = %d\r\n", frame->idx, buddy_f->idx, frame->order);

    return 1;
}

void page2chunk(int order)
{
    void *page = page_malloc(PAGE_SIZE);
    frame_t *frame = &frame_pool[((unsigned long long)page - BUDDY_MEMORY_START) >> 12];
    frame->chunk_order = order;

    int chunk_size = (CHUNK_SIZE << order);
    for (int i = 0; i < PAGE_SIZE; i += chunk_size) {
        struct list_head *c = (struct list_head *)(page + i);
        list_add_tail(c, &chunk[order]);
    }
}

void *chunk_malloc(unsigned int size)
{
    if (mem_print) {
        uart_printf("[+] Allocate chunk size : %d (0x%x)\r\n", size, size);
        uart_printf("    Before\r\n");
        dump_chunk_info();
    }

    int order;
    for (int i = 0; i <= CHUNK_MAX_ORDER; i++) {
        if (size <= (CHUNK_SIZE << i)) {
            order = i;
            break;
        }
    }

    if (list_empty(&chunk[order])) {
        page2chunk(order);
    }

    struct list_head *c = chunk[order].next;
    list_del(c);

    if (mem_print) {
        uart_printf("    physical address : 0x%x\n", (unsigned long long)c);
        uart_printf("    After\r\n");
        dump_chunk_info();
    }

    return (void *)c;
}

void chunk_free(void *addr)
{
    struct list_head *c = (struct list_head *)addr;
    frame_t *frame = &frame_pool[((unsigned long long)c - BUDDY_MEMORY_START) >> 12];

    if (mem_print) {
        uart_printf("[+] Free chunk address : 0x%x, order = %d\r\n", addr, frame->chunk_order);
        uart_printf("    Before\r\n");
        dump_chunk_info();
    }

    list_add_tail(c, &chunk[frame->chunk_order]);

    if (mem_print) {
        uart_printf("    After\r\n");
        dump_chunk_info();
    }
}

void dump_page_info()
{
    uart_printf("\t\t------ [  Number of Available Page Blocks  ] -------\r\n\t\t| ");
    for (int i = 0; i <= FRAME_MAX_ORDER; i++)
        uart_printf("%4dKB ", 4 * (1 << i));
    uart_printf("|\r\n\t\t| ");
    for (int i = 0; i <= FRAME_MAX_ORDER; i++)
        uart_printf("%6d ", list_size(&buddy[i]));
    uart_printf("|\r\n\t\t----------------------------------------------------\r\n");
}

void dump_chunk_info()
{
    uart_printf("\t------ [  Number of Available Chunk Slots  ] -------\r\n\t| ");
    for (int i = 0; i <= CHUNK_MAX_ORDER; i++)
        uart_printf("%5dB ", 32 * (1 << i));
    uart_printf("|\r\n\t| ");
    for (int i = 0; i <= CHUNK_MAX_ORDER; i++)
        uart_printf("%6d ", list_size(&chunk[i]));
    uart_printf("|\r\n\t----------------------------------------------------\r\n");
}

void *kmalloc(unsigned int size)
{
    if (mem_print) {
        uart_printf("\n\n================================\r\n");
        uart_printf("[+] Request size: %d\r\n", size);
        uart_printf("================================\r\n");
    }

    return (size > 0x800) ? page_malloc(size) : chunk_malloc(size);

    // if (size >= 0x800) {
    //     return page_malloc(size);
    // }
    // else {
    //     return chunk_malloc(size);
    // }
}

void kfree(void *addr)
{
    if (mem_print) {
        uart_printf("\n\n================================\r\n");
        uart_printf("[+] Request address: 0x%x\r\n", addr);
        uart_printf("================================\r\n");
    }

    return ((unsigned long long)addr % PAGE_SIZE == 0 &&
            frame_pool[((unsigned long long)addr - BUDDY_MEMORY_START) >> 12].chunk_order == CHUNK_NONE)
               ? page_free(addr)
               : chunk_free(addr);

    // if ((unsigned long long)addr % PAGE_SIZE == 0 &&
    //     frame_pool[((unsigned long long)addr - BUDDY_MEMORY_START) >> 12].chunk_order == CHUNK_NONE) {
    //     page_free(addr);
    // }
    // else {
    //     chunk_free(addr);
    // }
}

void memory_reserve(unsigned long long start, unsigned long long end)
{
    start -= start % PAGE_SIZE;
    end += (end % PAGE_SIZE) ? PAGE_SIZE - (end % PAGE_SIZE) : 0;

    if (mem_print)
        uart_printf("Reserved Memory 0x%x ~ 0x%x\n", start, end);

    for (int i = FRAME_MAX_ORDER; i >= 0; i--) {
        struct list_head *pos;

        list_for_each(pos, &buddy[i])
        {
            frame_t *pos_f = list_entry(pos, frame_t, list);
            unsigned long long addr_start = BUDDY_MEMORY_START + (PAGE_SIZE * pos_f->idx);
            unsigned long long addr_end = addr_start + (PAGE_SIZE << i);

            if (addr_start >= end || addr_end <= start) // Block is not reserved
                continue;
            else if (addr_start >= start && addr_end <= end) { // Block is fully reserved
                pos_f->used = FRAME_ALLOCATED;

                if (mem_print) {
                    uart_printf("\t[!] Reserved in 0x%x ~ 0x%x\n", addr_start, addr_end);
                    uart_printf("\t    Before\r\n");
                    dump_page_info();
                }

                list_del(pos);

                if (mem_print) {
                    uart_printf("\t    Removed order : %d\n", i);
                    uart_printf("\t    After\r\n");
                    dump_page_info();
                }
            }
            else { // Block is partially reserved
                struct list_head *prev_pos = pos->prev;
                list_del(pos);
                list_add_tail(&release_redundant(pos_f)->list, &buddy[i - 1]);
                pos = prev_pos;
            }
        }
    }
}

void page_test()
{
    mem_print = 1;

    // start
    char *a = kmalloc(0x1000);
    char *b = kmalloc(0x100);
    char *c = kmalloc(0x10);

    kfree(a);
    kfree(b);
    kfree(c);

    char *d = kmalloc(12345);
    char *e = kmalloc(4096);
    kfree(d);
    char *f = kmalloc(4095);
    char *g = kmalloc(4096);
    char *h = kmalloc(4096);
    char *i = kmalloc(8192);
    char *j = kmalloc(262143);
    kfree(f);
    kfree(g);
    kfree(h);
    kfree(e);
    kfree(i);
    kfree(j);
    char *k = kmalloc(4096);
    kfree(k);
    // end

    mem_print = 0;
}
