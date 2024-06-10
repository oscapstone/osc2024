#include "stdint.h"
#include "memory.h"
#include "mini_uart.h"
#include "list.h"
#include "exception.h"

extern char _start;
extern char _end;
extern char *CPIO_START;
extern char *CPIO_END;
extern char *DTB_START;
extern char *DTB_END;
extern char _heap_top;
static char* heaptop_ptr = &_heap_top;
static size_t memory_size = ALLOC_END - ALLOC_BASE;

/* ---- Lab2 ---- */
void* malloc(size_t size) {

    // -> heaptop_ptr
    //               header 0x10 bytes                   block
    //   ┌──────────────────────┬────────────────┬──────────────────────┐
    //   │  fill zero 0x8 bytes │ size 0x8 bytes │ size padding to 0x10 │
    //   └──────────────────────┴────────────────┴──────────────────────┘
    //

    // Add heap block header 16 bytes
    char* ptr = heaptop_ptr + 0x10;
    // Size paddling to multiple of 0x10
    size = size + 0x10 - size % 0x10;
    *(unsigned int*)(ptr - 0x8) = size;
    heaptop_ptr += size;    
    return ptr;
}

/* ---- Lab4 ---- */
static frame_t          *frame_array;
static list_head_t      frame_freelist[FRAME_MAX_IDX];  // store available block for page
static list_head_t      cache_list[CACHE_MAX_IDX];      // store available block for cache

void allocator_init() {   

    uart_puts("\r\n  ----------------- | Buddy System | Startup Allocation | -----------------\r\n");
    uart_puts("\r\n  memory size: 0x%x, max pages: %d, frame size: %d\r\n", memory_size, MAX_PAGE_COUNT, sizeof(frame_t));

    frame_array = malloc(MAX_PAGE_COUNT * sizeof(frame_t));

    /* init frame freelist */
    for (int i = 0; i <= FRAME_IDX_FINAL; i++) {
        INIT_LIST_HEAD(&frame_freelist[i]);
    }

    /* init cache list */
    for (int i = 0; i <= CACHE_IDX_FINAL; i++) {
        INIT_LIST_HEAD(&cache_list[i]);
    }

    /* init frame array */ 
    for (int i=0; i < MAX_PAGE_COUNT; i++) {

        INIT_LIST_HEAD(&(frame_array[i].listhead));
        frame_array[i].idx = i;
        frame_array[i].order = CACHE_NONE;

        /* split with max size frame (2^6) */
        if (i % (1 << FRAME_IDX_FINAL) == 0) {
            frame_array[i].val  = FRAME_IDX_FINAL;
            frame_array[i].used = FRAME_VAL_FREE;
            list_add(&frame_array[i].listhead, &frame_freelist[FRAME_IDX_FINAL]);
        }
    }
    uart_puts("\r\n");
    dump_page_info();
}

void* page_malloc(size_t size) {

    uart_puts("\r\n[+] Allocate page - size : %d(0x%x)\r\n", size, size);
    
    // Find the nearest val
    int val = val_to_frame_num(size);

    // find the smallest larger frame in freelist
    int target_val;
    for (target_val = val; target_val <= FRAME_IDX_FINAL; target_val++) {
        // freelist does not have 2**i order frame, going for next order
        if (!list_empty(&frame_freelist[target_val]))
            break;
    } 
    if (target_val > FRAME_IDX_FINAL) {
        uart_puts("[!] No available frame in freelist, page_malloc ERROR!!!!\r\n");
        return (void*)0;
    }

    // get the available frame from freelist
    frame_t *target_frame_ptr = (frame_t*)frame_freelist[target_val].next;
    list_del_entry((struct list_head *)target_frame_ptr);

    // Release redundant memory block to separate into pieces
    for (int j = target_val; j > val; j--) { // ex: 10000 -> 01111
        release_redundant_block(target_frame_ptr);
    }

    // Allocate it
    target_frame_ptr->used = FRAME_VAL_USED;
    uart_puts("    physical address : 0x%x\n", frame_addr_to_phy_addr(target_frame_ptr));
    uart_puts("    After\r\n");
    dump_page_info();

    return (void *)frame_addr_to_phy_addr(target_frame_ptr);
}

void page_free(void* ptr) {
    frame_t *target_frame_ptr = &frame_array[((unsigned long long)ptr - ALLOC_BASE) >> 12]; // PAGESIZE * Available Region -> 0x1000 * 0x10000000 // SPEC #1, #2
    uart_puts("  [+] Free page: 0x%x, val = %d\r\n",ptr, target_frame_ptr->val);
    uart_puts("      Before\r\n");
    dump_page_info();
    target_frame_ptr->used = FRAME_VAL_FREE;
    while(coalesce(target_frame_ptr)==0); // merge buddy iteratively
    list_add(&target_frame_ptr->listhead, &frame_freelist[target_frame_ptr->val]);
    uart_puts("      After\r\n");
    dump_page_info();
    return;
}

void* cache_malloc(unsigned int size) {
    uart_puts("[+] Allocate cache - size : %d(0x%x)\r\n", size, size);
    uart_puts("    Before\r\n");
    dump_cache_info();

    // turn size into cache order: 32B * 2**order
    int order;
    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++) {
        if (size <= (32 << i)) { order = i; break; }
    }

    // if no available cache in list, assign one page for it
    if (list_empty(&cache_list[order])) {
        page_to_caches(order);
    }

    list_head_t* r = cache_list[order].next;
    list_del_entry(r);
    uart_puts("    physical address : 0x%x\n", r);
    uart_puts("    After\r\n");
    dump_cache_info();
    return r;
}

void cache_free(void *ptr) {
    list_head_t *c = (list_head_t *)ptr;
    frame_t *pageframe_ptr = &frame_array[((unsigned long long)ptr - ALLOC_BASE) >> 12];
    uart_puts("[+] Free cache: 0x%x, val = %d\r\n",ptr, pageframe_ptr->order);
    uart_puts("    Before\r\n");
    dump_cache_info();
    list_add(c, &cache_list[pageframe_ptr->order]);
    uart_puts("    After\r\n");
    dump_cache_info();
}

frame_t* release_redundant_block(frame_t *frame) {
    // order -1 -> add its buddy to free list (frame itself will be used in master function)
    frame->val -= 1;
    frame_t *buddyptr = get_buddy(frame);
    buddyptr->val = frame->val;
    list_add(&buddyptr->listhead, &frame_freelist[buddyptr->val]);
    return frame;
}

frame_t* get_buddy(frame_t *frame) {
    // XOR(idx, order)
    uart_puts("index: %d, order: %d\r\n", frame->idx, (1 << frame->val));
    uart_puts("buddy index: %d\r\n", frame->idx ^ (1 << frame->val));
    return &frame_array[frame->idx ^ (1 << frame->val)];
}

int coalesce(frame_t *frame_ptr) {
    frame_t *buddy = get_buddy(frame_ptr);
    // frame is the boundary
    if (frame_ptr->val == FRAME_IDX_FINAL)
        return -1;

    // Order must be the same: 2**i + 2**i = 2**(i+1)
    if (frame_ptr->val != buddy->val)
        return -1;

    // buddy is in used
    if (buddy->used == FRAME_VAL_USED)
        return -1;

    list_del_entry((struct list_head *)buddy);
    frame_ptr->val += 1;
    uart_puts("      coalesce detected, merging 0x%x, 0x%x, -> val = %d\r\n", frame_ptr->idx, buddy->idx, frame_ptr->val);
    return 0;
}

int val_to_frame_num(size_t val) {
    /* Convert size to minimum frame unit (4KB) x 2^val */
    for (size_t i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++) {
        if (val <= (PAGE_SIZE << i)) {
            val = i;
            uart_puts("    Will allocate frame size 0x%x\r\n", PAGE_SIZE << i);
            break;
        }
        if (i == FRAME_IDX_FINAL) {
            uart_puts("    Request size exceeded for page_malloc!\r\n", PAGE_SIZE << i);
            return -1;
        }
    }
    return val;
}

void *kmalloc(size_t size) {
    if (size > (32 << CACHE_IDX_FINAL)) {
        void *ptr = page_malloc(size);
        return ptr;

    }
    void *ptr = cache_malloc(size);
    return ptr;
}

void kfree(void *ptr) {
    // If no cache assigned, go for page
    if ((unsigned long long)ptr % PAGE_SIZE == 0 && frame_array[((unsigned long long)ptr - ALLOC_BASE) >> 12].order == CACHE_NONE) {
        page_free(ptr);
        return;
    }
    // go for cache
    cache_free(ptr);
}

void dump_page_info(){
    unsigned int exp2 = 1;
    uart_puts("  ---------------------- |  Available Page Blocks  | ----------------------\r\n  | ");
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++) {
        uart_puts("%4dKB(%1d) ", 4*exp2, i);
        exp2 *= 2;
    }
    uart_puts("|\r\n  | ");
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
        uart_puts("     %4d ", list_size(&frame_freelist[i]));
    uart_puts("|\r\n");
    uart_puts("  -------------------------------------------------------------------------\r\n");

}

void dump_cache_info() {
    unsigned int exp2 = 1;
    uart_puts("  ---------------------- |  Available Cache Blocks  | ---------------------\r\n  | ");
    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
    {
        uart_puts("%4dB(%1d)  ", 32*exp2, i);
        exp2 *= 2;
    }
    uart_puts("|\r\n  | ");
    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
        uart_puts("    %4d  ", list_size(&cache_list[i]));
    uart_puts("|\r\n");
    uart_puts("  -------------------------------------------------------------------------\r\n");

}

void page_to_caches(int order) {
    // make caches from a smallest-size page
    char *page = page_malloc(PAGE_SIZE);
    frame_t *pageframe_ptr = &frame_array[((unsigned long long)page - ALLOC_BASE) >> 12];
    pageframe_ptr->order = order;

    // split page into a lot of caches and push them into cache_list
    int cachesize = (32 << order);
    for (int i = 0; i < PAGE_SIZE; i += cachesize)
    {
        list_head_t *c = (list_head_t *)(page + i);
        list_add(c, &cache_list[order]);
    }
}

void memory_reserve(unsigned long long start, unsigned long long end) {
    start -= start % PAGE_SIZE; // floor (align 0x1000)
    end = end % PAGE_SIZE ? end + PAGE_SIZE - (end % PAGE_SIZE) : end; // ceiling (align 0x1000)

    uart_puts("Reserved Memory: ");
    uart_puts("start 0x%x ~ ", start);
    uart_puts("end 0x%x\r\n",end);

    // delete page from free list
    for (int order = FRAME_IDX_FINAL; order >= 0; order--) {
        list_head_t *pos;
        list_for_each(pos, &frame_freelist[order]) {
            unsigned long long pagestart = ((frame_t *)pos)->idx * PAGE_SIZE + ALLOC_BASE;
            unsigned long long pageend = pagestart + (PAGE_SIZE << order);

            if (start <= pagestart && end >= pageend) { // if page all in reserved memory -> delete it from freelist
                ((frame_t *)pos)->used = FRAME_VAL_USED;
                uart_puts("    [!] Reserved page in 0x%x - 0x%x\n", pagestart, pageend);
                uart_puts("        Before\n");
                dump_page_info();
                list_del_entry(pos);
                uart_puts("        Remove usable block for reserved memory: order %d\r\n", order);
                uart_puts("        After\n");
                dump_page_info();
            } else if (start >= pageend || end <= pagestart) { 
                // no intersection
                continue;
            } else { 
                // partial intersection, separate the page into smaller size.
                list_del_entry(pos);
                list_head_t *temppos = pos -> prev;
                list_add(&release_redundant_block((frame_t *)pos)->listhead, &frame_freelist[order - 1]);
                pos = temppos;
            }
        }
    }
}

void memory_init() {
    allocator_init();
    memory_reserve(0x0000, 0x1000); // Spin tables for multicore boot (0x0000 - 0x1000)
    uart_puts("_start: 0x%x, _end: 0x%x\n", &_start, &_end);
    memory_reserve((size_t)&_start, (size_t)&_end);
    uart_puts("CPIO_START: 0x%x, CPIO_END: 0x%x\n", CPIO_START, CPIO_END);
    memory_reserve((size_t)CPIO_START, (size_t)CPIO_END);
    uart_puts("DTB_START: 0x%x, DTB_END: 0x%x\n", DTB_START, DTB_END);
    memory_reserve((size_t)DTB_START, (size_t)DTB_END);
}
      

size_t frame_to_index(frame_t *frame) {
    return (size_t)((frame_t *)frame - frame_array);
}

frame_t *index_to_frame(size_t index) {
    return &frame_array[index];
}

frame_t *phy_addr_to_frame(void *ptr) {
    return (frame_t *)&frame_array[(uint64_t)ptr / PAGE_SIZE];
}

size_t frame_addr_to_phy_addr(frame_t *frame) {
    return ALLOC_BASE + (frame_to_index(frame) * PAGE_SIZE);
}

