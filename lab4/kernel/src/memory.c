#include "memory.h"
#include "u_list.h"
#include "uart1.h"
#include "exception.h"
#include "dtb.h"

extern char  _heap_top;
static char *htop_ptr = &_heap_top;

extern char  _start;
extern char  _end;
extern char  _stack_top;
extern char *CPIO_DEFAULT_START;
extern char *CPIO_DEFAULT_END;
extern char *dtb_ptr;

// ------ Lab2 ------
void* init_malloc(unsigned int size) {
    // -> htop_ptr
    // htop_ptr + 0x02:  heap_block size
    // htop_ptr + 0x10 ~ htop_ptr + 0x10 * k:
    //            { heap_block }
    // -> htop_ptr

    // 0x10 for heap_block header
    char* r = htop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned int*)(r - 0x8) = size;
    htop_ptr += size;
    return r;
}

void init_free(void* ptr) {
    // Do it latter
}

// ------ Lab4 ------
static frame_t            *frame_array;                    // store memory's header
static list_head_t        frame_freelist[FRAME_MAX_IDX];  // store available/free block for page
static list_head_t        cache_list[CACHE_MAX_IDX];      // store available/free block for cache

void init_allocator()
{
    frame_array = init_malloc(BUDDY_MEMORY_PAGE_COUNT * sizeof(frame_t));

    // init frame_array
    for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i++)
        if (i % (1 << FRAME_IDX_FINAL) == 0) {
            frame_array[i].val = FRAME_IDX_FINAL;
            frame_array[i].used = FRAME_FREE;
        }

    //init frame freelist
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
        INIT_LIST_HEAD(&frame_freelist[i]);

    //init cache list
    for (int i = CACHE_IDX_0; i<= CACHE_IDX_FINAL; i++)
        INIT_LIST_HEAD(&cache_list[i]);

    for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i++) {
        // init listhead for each frame
        INIT_LIST_HEAD(&frame_array[i].listhead);
        frame_array[i].idx = i;
        frame_array[i].cache_order = CACHE_NONE;

        // add init frame (FRAME_IDX_FINAL) into freelist
        if (i % (1 << FRAME_IDX_FINAL) == 0)
            list_add(&frame_array[i].listhead, &frame_freelist[FRAME_IDX_FINAL]);
    }

    /* Startup reserving the following region:
    Spin tables for multicore boot (0x0000 - 0x1000)
    Devicetree (Optional, if you have implement it)
    Kernel image in the physical memory
    Your simple allocator (startup allocator) (Stack + Heap in my case)
    Initramfs
    */
    uart_sendline("\r\n* Startup Allocation *\r\n");
    uart_sendline("buddy system: usable memory region: 0x%x ~ 0x%x\n", BUDDY_MEMORY_BASE, BUDDY_MEMORY_BASE + BUDDY_MEMORY_PAGE_COUNT * PAGESIZE);
    dtb_find_and_store_reserved_memory();                                                                                     // find spin tables in dtb
    memory_reserve((unsigned long long)&_start,             (unsigned long long)&_end,              "kernel code & data");    // kernel code and data
    memory_reserve((unsigned long long)CPIO_DEFAULT_START,  (unsigned long long)CPIO_DEFAULT_END,   "CPIO");                  // CPIO
}

void* page_malloc(unsigned int size)
{
    uart_sendline("    [+] Allocate page - size : %d(0x%x)\r\n", size, size);
    dump_page_info("Before");

    int val;
    // turn size into minimum 4KB * (2**val), to find the right value
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++){
        if (size <= (PAGESIZE << i)) {
            val = i;
            uart_sendline("        block size = 0x%x, val = %d\n", PAGESIZE << i, i);
            break;
        }

        if ( i == FRAME_IDX_FINAL) {
            uart_puts("[!] request size exceeded for page_malloc!!!!\r\n");
            return (void*)0;
        }

    }

    // find the available frame in freelist
    int target_val;
    for (target_val = val; target_val <= FRAME_IDX_FINAL; target_val++) {
        if (!list_empty(&frame_freelist[target_val]))
            break;
    }

    // we don't find any available frame in freelist
    if (target_val > FRAME_IDX_FINAL) {
        uart_puts("[!] No available frame in freelist, page_malloc ERROR!!!!\r\n");
        return (void*)0;
    }

    // get the available frame from freelist
    frame_t *target_frame_ptr = (frame_t*)frame_freelist[target_val].next;
    list_del_entry((struct list_head *)target_frame_ptr);

    // Release redundant memory block to separate into pieces
    for (int j = target_val; j > val; j--) // ex: 10000 -> 01111
        release_redundant(target_frame_ptr);

    // Allocate it
    target_frame_ptr->used = FRAME_ALLOCATED;
    uart_sendline("        physical address : 0x%x\n", PAGE_INDEX_TO_PTR(target_frame_ptr->idx));
    dump_page_info("After");

    return PAGE_INDEX_TO_PTR(target_frame_ptr->idx);
}

void page_free(void* ptr)
{
    frame_t *target_frame_ptr = &frame_array[PTR_TO_PAGE_INDEX(ptr)]; // PAGESIZE * Available Region -> 0x1000 * 0x10000000 // SPEC #1, #2
    
    uart_sendline("    [+] Free page: 0x%x, val = %d\r\n",ptr, target_frame_ptr->val);
    dump_page_info("Before");
    target_frame_ptr->used = FRAME_FREE;
    while(coalesce(&target_frame_ptr) != -1); // merge buddy iteratively

    list_add(&target_frame_ptr->listhead, &frame_freelist[target_frame_ptr->val]);
    dump_page_info("After");
}


frame_t* release_redundant(frame_t *frame)
{
    // order -1 -> add its buddy to free list (frame itself will be used in master function)
    frame->val -= 1;
    frame_t *buddyptr = GET_BUDDY(frame);
    buddyptr->val = frame->val;
    list_add(&buddyptr->listhead, &frame_freelist[buddyptr->val]);
    return frame;
}

int coalesce(frame_t **_frame_ptr)
{
    frame_t *frame_ptr = *_frame_ptr;
    frame_t *buddy = GET_BUDDY(frame_ptr);
    // frame is the boundary
    if (frame_ptr->val == FRAME_IDX_FINAL)
        return -1;

    // Order must be the same: 2**i + 2**i = 2**(i+1)
    if (frame_ptr->val != buddy->val)
        return -1;

    // buddy is in used
    if (buddy->used == FRAME_ALLOCATED)
        return -1;
    list_del_entry((struct list_head *)buddy);
    uart_sendline("   {before changing} coalesce detected, merging frame index %d (0x%x), %d (0x%x), -> val = %d\r\n", 
        frame_ptr->idx,     PAGE_INDEX_TO_PTR(frame_ptr->idx), 
        buddy->idx,         PAGE_INDEX_TO_PTR(buddy->idx),
        frame_ptr->val
    );
    *_frame_ptr = frame_ptr->idx > buddy->idx ? buddy : frame_ptr; // we want to find the smaller index and make it as the new head
    (*_frame_ptr)->val += 1;
    (*_frame_ptr)->used = FRAME_FREE;
    (*_frame_ptr)->cache_order = CACHE_NONE;
    buddy = frame_ptr->idx > buddy->idx ? frame_ptr : buddy;
    
    uart_sendline("   {after changing}  coalesce detected, merging frame index %d (0x%x), %d (0x%x), -> val = %d\r\n", 
        (*_frame_ptr)->idx,     PAGE_INDEX_TO_PTR((*_frame_ptr)->idx), 
        buddy->idx,             PAGE_INDEX_TO_PTR(buddy->idx),
        (*_frame_ptr)->val
    );
    
    return 0;
}

void dump_page_info(char *msg){
    unsigned int exp2 = 1;
    uart_sendline("        ------------- [ {%s} Number of Available Page Blocks  ] -------------\r\n        | ", msg);
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++) {
        uart_sendline("%4dKB(%1d) ", 4*exp2, i);
        exp2 *= 2;
    }
    uart_sendline("|\r\n        | ");
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
        uart_sendline("     %4d ", list_size(&frame_freelist[i]));
    uart_sendline("|\r\n");
    uart_sendline("        -------------------------------------------------------------------------\r\n");
}

void dump_cache_info(char *msg)
{
    unsigned int exp2 = 1;
    uart_sendline("    ---------- [ {%s} Number of Available Cache Blocks ] --------- \r\n    | ", msg);
    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++){
        uart_sendline("%4dB(%1d) ", 32*exp2, i);
        exp2 *= 2;
    }
    uart_sendline("|\r\n    | ");
    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
        uart_sendline("   %5d ", list_size(&cache_list[i]));
    uart_sendline("|\r\n");
    uart_sendline("    -----------------------------------------------------------------\r\n", msg);
    // uart_sendline("    -------------------------------------------\r\n");
}

void page2caches(int order)
{
    // make caches from a smallest-size page
    char *page = page_malloc(PAGESIZE);
    // frame_t *pageframe_ptr = &frame_array[((unsigned long long)page - BUDDY_MEMORY_BASE) >> 12];
    frame_t *pageframe_ptr = &frame_array[PTR_TO_PAGE_INDEX(page)];
    pageframe_ptr->cache_order = order;

    // split page into a lot of caches and push them into cache_list
    int cachesize = (32 << order);
    for (int i = 0; i < PAGESIZE; i += cachesize) {
        list_head_t *c = (list_head_t *)(page + i);
        list_add(c, &cache_list[order]);
    }
}

void* cache_malloc(unsigned int size)
{
    uart_sendline("[+] Allocate cache - size : %d(0x%x)\r\n", size, size);
    dump_cache_info("Before");

    // turn size into cache order: 32B * 2**order
    int order;
    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
        if (size <= (32 << i)) { 
            order = i; 
            break; 
        }

    // if no available cache in list, assign one page for it
    if (list_empty(&cache_list[order]))
        page2caches(order);

    list_head_t* r = cache_list[order].next;
    list_del_entry(r);
    uart_sendline("    physical address : 0x%x\n", r);
    dump_cache_info("After");
    return r;
}

void cache_free(void *ptr)
{
    list_head_t *c = (list_head_t *)ptr;
    // frame_t *pageframe_ptr = &frame_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12];
    frame_t *pageframe_ptr = &frame_array[PTR_TO_PAGE_INDEX(ptr)];
    uart_sendline("[-] Free cache: 0x%x, val = %d\r\n",ptr, pageframe_ptr->cache_order);
    dump_cache_info("Before");
    list_add(c, &cache_list[pageframe_ptr->cache_order]);
    dump_cache_info("After");
}

void *kmalloc(unsigned int size)
{
    uart_sendline("\n\n");
    uart_sendline("================================\r\n");
    uart_sendline("{+} Request kmalloc size: %d\r\n", size);
    uart_sendline("================================\r\n");
    // if size is larger than cache size, go for page
    if (size > (32 << CACHE_IDX_FINAL))
    {
        void *r = page_malloc(size);
        return r;
    }
    // go for cache
    void *r = cache_malloc(size);
    return r;
}

void kfree(void *ptr)
{
    uart_sendline("\n\n");
    uart_sendline("==========================\r\n");
    uart_sendline("{-} Request kfree 0x%x\r\n", ptr);
    uart_sendline("==========================\r\n");
    // If no cache assigned, go for page
    if ((unsigned long long)ptr % PAGESIZE == 0 && frame_array[PTR_TO_PAGE_INDEX(ptr)].cache_order == CACHE_NONE)
    {
        page_free(ptr);
        return;
    }
    // go for cache
    cache_free(ptr);
}

void memory_reserve(unsigned long long start, unsigned long long end, char *name)
{
    if (start >= end)
        return;
    start   -= start % PAGESIZE; // floor (align 0x1000)
    end     = end % PAGESIZE ? end + PAGESIZE - (end % PAGESIZE) : end; // ceiling (align 0x1000)

    uart_sendline("=========================================================================================\n");
    uart_sendline("* %s *\r\n", name);
    uart_sendline("Reserved Memory: ");
    uart_sendline("0x%x ~ ", start);
    uart_sendline("0x%x\r\n",end);
    // delete page from free list, using recusive function
    _memory_reserve(start, end, FRAME_IDX_FINAL);
}

void _memory_reserve(unsigned long long start, unsigned long long end, int order)
{
    // delete page from free list
    for (; order >= 0; order--) {
        list_head_t *pos;
        list_for_each(pos, &frame_freelist[order]) {
            unsigned long long pagestart = ((frame_t *)pos)->idx * PAGESIZE + BUDDY_MEMORY_BASE;
            unsigned long long pageend = pagestart + (PAGESIZE << order);

            /*
                (start) --- (pagestart) --- (pageend) --- (end)
            */
            if (start <= pagestart && end >= pageend) { // if page all in reserved memory -> delete it from freelist
                ((frame_t *)pos)->used = FRAME_ALLOCATED;
                uart_sendline("    [!] Reserved page in 0x%x - 0x%x\n", pagestart, pageend);
                dump_page_info("Before");
                list_del_entry(pos);
                uart_sendline("        Remove usable block for reserved memory: order %d\r\n", order);
                dump_page_info("After");

                _memory_reserve(start, pagestart, order);
                _memory_reserve(pageend, end, order);
                break;
            }

            /*
                (pagestart) --- (pageend) --- (start)--- (end)
                (start)--- (end) --- (pagestart) --- (pageend) 
            */
            else if (start >= pageend || end <= pagestart) // no intersection
                continue;
                
            /*
                partial intersection, separate the page into smaller size.
                (pagestart) --- (start) --- (end) --- (pageend) 
            */
            else { 
                uart_sendline("    [!] partial intersection in 0x%x - 0x%x\n", pagestart, pageend);
                dump_page_info("Before");
                list_del_entry(pos);
                list_head_t *temp_pos = pos -> prev;
                list_add(&release_redundant((frame_t *)pos)->listhead, &frame_freelist[order - 1]);
                pos = temp_pos;
                dump_page_info("After");
            }
        }
    }
}
