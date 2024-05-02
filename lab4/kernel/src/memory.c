#include "memory.h"
#include "mbox.h"
#include "stdio.h"
#include "uart1.h"
#include "list.h"
#include "exception.h"

// Address           0k    4k    8k    12k   16k   20k   24k   28k   32k   36k   40k   44k   48k   52k   56k   60k
//                   ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
// Frame Array(val)  │  3  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <0> │  0  │  1  │ <F> │  2  │ <F> │ <F> │ <F> │
//                   └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
// index                0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
//
// val >=0: frame is free, size = 2^val * 4k

#define F_FRAME_VAL -1 // This frame is free, but it belongs to a larger contiguous memory block.
// #define X_FRAME_VAL -2 // This frame is already allocated.

extern uint8_t _heap_top;
extern char _start;
extern char _end;
extern char *CPIO_START;
extern char *CPIO_END;
extern char *DTB_START;
extern char *DTB_END;
static uint8_t *khtop_ptr = &_heap_top;
static size_t memory_size;
static size_t max_frame;
static frame_t *frame_array;
static list_head_t *frame_freelist[MAX_VAL + 1];
static list_head_t *cache_freelist[MAX_ORDER + 1];

/*
 * val to number of frame
 */
static inline size_t val_to_num_of_frame(int8_t val)
{
    return 1 << val;
}

static inline size_t frame_to_index(frame_t *frame)
{
    return (size_t)((frame_t *)frame - frame_array);
}

static inline frame_t *phy_addr_to_frame(void *ptr)
{
    return (frame_t *)&frame_array[(unsigned long)ptr / PAGE_FRAME_SIZE];
}

static inline void *frame_addr_to_phy_addr(frame_t *frame)
{
    return (void *)(frame_to_index(frame) * PAGE_FRAME_SIZE);
}

static inline frame_t *find_end_frame_of_block(frame_t *start_frame, int8_t val)
{
    return start_frame + val_to_num_of_frame(val) - 1;
}

static inline frame_t *index_to_frame(size_t index)
{
    return &frame_array[index];
}

static inline frame_t *cache_to_frame(void *ptr)
{
    return phy_addr_to_frame(ptr);
}

static inline size_t cache_to_index(void *ptr)
{
    frame_t *frame = cache_to_frame(ptr);
    return ((uint64_t)ptr - (uint64_t)frame) / (1 << (BASE_ORDER + frame->order));
}

static inline size_t order_to_size(int8_t order)
{
    return 1 << (BASE_ORDER + order);
}

static inline size_t get_buddy_index(char val, size_t index)
{
    // XOR(val, index)
    return index ^ val_to_num_of_frame(val);
}

static inline frame_t *get_buddy_frame(char val, frame_t *frame)
{
    // XOR(val, index)
    return &(frame_array[get_buddy_index(val, frame_to_index(frame))]);
}

static void *startup_malloc(size_t size)
{
    // -> khtop_ptr
    //               header 0x10 bytes                   block
    // ┌──────────────────────┬────────────────┬──────────────────────┐
    // │  fill zero 0x8 bytes │ size 0x8 bytes │ size padding to 0x16 │
    // └──────────────────────┴────────────────┴──────────────────────┘
    //
    uart_puts("begin khtop_ptr: 0x%x\n", khtop_ptr);
    void *sp;
    asm("mov %0, sp" : "=r"(sp));
    // 0x10 for heap_block header
    uint8_t *r = khtop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(uint64_t *)(r - 0x8) = size;
    khtop_ptr += size;
    uart_puts("khtop_ptr: 0x%x, sp: 0x%x\n", khtop_ptr, sp);
    return r;
}

static void startup_free(void *ptr)
{
    // TBD
}

static int page_insert(char val, frame_t *frame)
{
    // list_head_t *curr;
    // list_for_each(curr, frame_freelist[(size_t)val])
    // {
    //     if ((frame_t *)curr > frame)
    //     {
    //         list_add((list_head_t *)frame, curr->prev);
    //         return 0;
    //     }
    // }
    // uart_puts("page_insert: frame: 0x%x, frame_freelist[%d]: 0x%x\n", frame, val,frame_freelist[(size_t)val]);
    list_add((list_head_t *)frame, frame_freelist[(size_t)val]);
    return 0;
}

static int allocate_frame_buddy(int begin_frame, int count, char val)
{
    int size = val_to_num_of_frame(val);
    for (size_t i = 0; i < count; i++)
    {
        int curr_buddy = begin_frame + size * i;
        frame_array[curr_buddy].val = val;
        for (size_t j = 1; j < size; j++)
            frame_array[curr_buddy + j].val = F_FRAME_VAL;
        list_add_tail((list_head_t *)index_to_frame(curr_buddy), frame_freelist[(size_t)val]);
    }
    return 0;
}

static int allocate_frame()
{
    max_frame = memory_size / PAGE_FRAME_SIZE; // each frame is 4KB
    uart_puts("max_frame: 0x%x, frame_size: %d, list_head size: %d, int8_t size: %d, uint8_t size: %d\n", max_frame, sizeof(frame_t)); // 24 Bytes
    frame_array = (frame_t *)startup_malloc(max_frame * sizeof(frame_t)); // in 0x3C000000 Bytes Ram, array size is 0x5A0030 Bytes
    int begin_frame = 0;
    for (size_t i = MAX_VAL; i >= 0; i--)
    {
        frame_freelist[i] = (list_head_t *)startup_malloc(sizeof(list_head_t));
        INIT_LIST_HEAD(frame_freelist[i]);
        int count = (max_frame - begin_frame) / (1 << i);
        allocate_frame_buddy(begin_frame, count, i);
        begin_frame += count * (1 << i);
    }
    return 0;
}

void *kmalloc(size_t size)
{
    lock();
    if (size >= PAGE_FRAME_SIZE / 2)
    {
        void *ptr = page_malloc(size);
        // uart_puts("use page_malloc: ptr: 0x%x, frame->order: %d, frame->cache_used_count: %d, frame->val: %d\n", ptr, phy_addr_to_frame(ptr)->order, phy_addr_to_frame(ptr)->cache_used_count, phy_addr_to_frame(ptr)->val);
        // dump_cache();
        unlock();
        return ptr;
    }
    int8_t order = 0;
    while (order_to_size(order) < size)
    {
        order++;
    }
    // uart_puts("kmalloc: size: 0x%x, order: %d, cache_size: 0x%x\n", size, order, order_to_size(order));
    struct list_head *curr;
    size_t index = -1;
    if (!list_empty(cache_freelist[(size_t)order])) // find the exact size
    {
        curr = cache_freelist[(size_t)order]->next;
        list_del_entry(curr);
        curr->next = curr->prev = curr;
        phy_addr_to_frame(curr)->cache_used_count++;
        // uart_puts("kmalloc: find the exact size, cache address: 0x%x, frame->cache_used_count: %d, frame->val: %d, frame address: 0x%x\n", curr, phy_addr_to_frame(curr)->cache_used_count, phy_addr_to_frame(curr)->val, phy_addr_to_frame(curr));
        // uart_puts("sizeof(list_head_t): 0x%x, return: 0x%x\r\n", sizeof(list_head_t), (void *)curr + sizeof(list_head_t));
        // dump_cache();
        unlock();
        return (void *)curr;
    }

    void *ptr = page_malloc(PAGE_FRAME_SIZE);
    frame_t *frame = phy_addr_to_frame(ptr);
    // uart_puts("kmalloc: split a new frame, ptr address: 0x%x\n", ptr);
    frame->order = order;
    frame->cache_used_count = 1;
    size_t order_size = order_to_size(order);
    for (size_t i = 1; i < PAGE_FRAME_SIZE / order_size; i++)
    {
        list_add((list_head_t *)(ptr + i * order_size), cache_freelist[(size_t)order]);
    }
    // dump_cache();
    unlock();
    return ptr;
}

void kfree(void *ptr)
{
    lock();
    frame_t *frame = cache_to_frame(ptr);
    // uart_puts("kfree: address: 0x%x, frame->order: %d, frame->cache_used_count: %d, frame->val: %d\n", ptr, frame->order, frame->cache_used_count, frame->val);
    if (frame->order == NOT_CACHE)
    {
        // uart_puts("kfree: page_free: 0x%x\n", ptr);
        page_free(ptr);
        // dump_cache();
        unlock();
        return;
    }
    frame->cache_used_count--;
    // uart_puts("kfree: cache address: 0x%x, frame->order: %d, frame->cache_used_count: %d, frame->val: %d\n", ptr, frame->order, frame->cache_used_count, frame->val);
    size_t order_size = order_to_size(frame->order);
    if (frame->cache_used_count == 0)
    {
        // uart_puts("kfree: cache_used_count == 0, free frame: 0x%x\n", frame);
        // uart_puts("remove the cache with the same frame from freelist\r\n");
        for (size_t i = 1; i < PAGE_FRAME_SIZE / order_size; i++)
        {
            list_del_entry((list_head_t *)((void *)frame_addr_to_phy_addr(frame) + i * order_size));
        }
        frame->order = NOT_CACHE;
        page_free(ptr);
        // dump_cache();
        unlock();
        return;
    }
    // uart_puts("kfree: cache_used_count != 0, add to cache_freelist: 0x%x\n", ptr);
    list_add((list_head_t *)ptr, cache_freelist[(size_t)frame->order]);
    // uart_puts("add finish\r\n");
    // dump_cache();
    unlock();
    return;
}

int memory_reserve(size_t start, size_t end)
{
    lock();
    size_t start_index = start / PAGE_FRAME_SIZE;                                    // align
    size_t end_index = end / PAGE_FRAME_SIZE + (end % PAGE_FRAME_SIZE == 0 ? 0 : 1); // padding
    // split the start frame to fit the start address
    frame_t *start_frame = index_to_frame(start_index);
    frame_t *end_frame = index_to_frame(end_index);
    size_t curr_index = start_index;
    uart_puts("start reserve: start_index: (0x%x, 0x%x), end_index: (0x%x, 0x%x)\n", start_index, frame_addr_to_phy_addr(start_frame), end_index, frame_addr_to_phy_addr(end_frame));

    while (index_to_frame(curr_index)->val == F_FRAME_VAL)
    {
        curr_index--;
    }
    while (curr_index <= end_index)
    {
        frame_t *curr_frame = index_to_frame(curr_index);
        if(list_empty((list_head_t *)curr_frame)){
            uart_puts("curr_frame is allocated, curr_index: (0x%x, 0x%x), curr_frame->val: %d\n", curr_index, frame_addr_to_phy_addr(curr_frame), curr_frame->val);
            while (1)
                ;
        }
        if (curr_index < start_index)
        {
            uart_puts("curr_index is before start_index, curr_index: (0x%x, 0x%x), start_index: (0x%x, 0x%x)\n", curr_index, frame_addr_to_phy_addr(curr_frame), start_index, frame_addr_to_phy_addr(start_frame));
            size_t split_val = curr_frame->val - 1;
            size_t buddy_index = get_buddy_index(split_val, curr_index);
            frame_t *buddy_frame = index_to_frame(buddy_index);
            curr_frame->val = split_val;
            buddy_frame->val = split_val;
            list_del_entry((list_head_t *)curr_frame);
            page_insert(split_val, curr_frame);
            page_insert(split_val, buddy_frame);
            if (buddy_index <= start_index)
            {
                uart_puts("curr_index = buddy_index, buddy_index: (0x%x, 0x%x), buddy_index->val: %d\n", buddy_index, frame_addr_to_phy_addr(buddy_frame), buddy_frame->val);
                curr_index = buddy_index;
            }
        }
        else if (start_index <= curr_index && curr_index + val_to_num_of_frame(curr_frame->val) - 1 <= end_index)
        {
            uart_puts("reserve frame: (0x%x -> 0x%x), curr_frame->val: %d\n", frame_addr_to_phy_addr(curr_frame), frame_addr_to_phy_addr(curr_frame + val_to_num_of_frame(curr_frame->val)), curr_frame->val);
            list_del_entry((list_head_t *)curr_frame);
            curr_index += val_to_num_of_frame(curr_frame->val);
        }
        else if (start_index <= curr_index && end_index < curr_index + val_to_num_of_frame(curr_frame->val) - 1)
        {
            uart_puts("curr_index is over end_index, curr_index: %d, end_index: %d, curr->val: %d, curr: (0x%x -> 0x%x), end: (0x%x)\n", curr_index, end_index, curr_frame->val, frame_addr_to_phy_addr(curr_frame), frame_addr_to_phy_addr(curr_frame + val_to_num_of_frame(curr_frame->val)), frame_addr_to_phy_addr(end_frame));
            size_t split_val = curr_frame->val - 1;
            size_t buddy_index = get_buddy_index(split_val, curr_index);
            frame_t *buddy_frame = index_to_frame(buddy_index);
            curr_frame->val = split_val;
            buddy_frame->val = split_val;
            uart_puts("split: (0x%x -> 0x%x), buddy: (0x%x -> 0x%x)\n", frame_addr_to_phy_addr(curr_frame), frame_addr_to_phy_addr(curr_frame + val_to_num_of_frame(curr_frame->val)), frame_addr_to_phy_addr(buddy_frame), frame_addr_to_phy_addr(buddy_frame + val_to_num_of_frame(buddy_frame->val)));
            list_del_entry((list_head_t *)curr_frame);
            page_insert(split_val, curr_frame);
            page_insert(split_val, buddy_frame);
        }
        else
        {
            uart_puts("something wrong\r\n");
            while (1)
                ;
        }
    }
    uart_puts("end reserve: (0x%x -> 0x%x)\n", frame_addr_to_phy_addr(start_frame), frame_addr_to_phy_addr(end_frame + 1));
    unlock();
    return 0;
}

size_t get_memory_size()
{
    // print arm memory
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_ARM_MEMORY;
    pt[3] = 8;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;
    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
        return (size_t)pt[6];
    else
        return 0;
}

void memory_init()
{
    memory_size = get_memory_size();
    allocate_frame();
    init_cache();

// dump_frame();
#define TEST_NUM 4
    // int size[TEST_NUM] = {5, 2, 43, 50};
    // int malloc_order[TEST_NUM] = {0, 3, 2, 1};
    // void *ptr[TEST_NUM];
    // int free_order[4] = {1, 3, 2, 0};
    // void *frame_size_page = page_malloc(PAGE_FRAME_SIZE);
    // uart_puts("malloc: 0x%x Bytes, address: 0x%x\n", PAGE_FRAME_SIZE, frame_size_page);
    // dump_frame();
    // for (size_t i = 0; i < TEST_NUM; i++)
    // {
    //     ptr[malloc_order[i]] = page_malloc(size[malloc_order[i]] * 1024);
    //     uart_puts("malloc: 0x%x Bytes, address: 0x%x\n", size[malloc_order[i]] * 1024, ptr[malloc_order[i]]);
    //     dump_frame();
    // }
    // for (size_t i = 0; i < TEST_NUM; i++)
    // {
    //     uart_puts("free: 0x%x Bytes, address: 0x%x\n", size[free_order[i]] * 1024, ptr[free_order[i]]);
    //     page_free(ptr[free_order[i]]);
    //     dump_frame();
    // }
    // uart_puts("malloc: 0x%x Bytes, address: 0x%x\n", PAGE_FRAME_SIZE, frame_size_page);
    // page_free(frame_size_page);
    // dump_frame();
    // uart_puts("Frame END\r\n");
    int size[TEST_NUM] = {5, 20, 430, 5900};
    int malloc_order[TEST_NUM] = {0, 3, 2, 1};
    void *ptr[TEST_NUM];
    int free_order[TEST_NUM] = {1, 3, 2, 0};
    // dump_cache();
    for (size_t i = 0; i < TEST_NUM; i++)
    {
        ptr[malloc_order[i]] = kmalloc(size[malloc_order[i]]);
        *(int *)(ptr[malloc_order[i]]) = 999999999;
        uart_puts("kmalloc_: 0x%x Bytes, address: 0x%x\n", size[malloc_order[i]], ptr[malloc_order[i]]);
        dump_frame();
        dump_cache();
    }
    for (size_t i = 0; i < TEST_NUM; i++)
    {
        for (size_t j = 0; j < TEST_NUM; j++)
        {
            uart_puts("ptr[%d]: 0x%x\r\n", j, ptr[j]);
        }
        uart_puts("kfree_: 0x%x Bytes, address: 0x%x\n", size[free_order[i]], ptr[free_order[i]]);
        kfree(ptr[free_order[i]]);
        dump_frame();
        dump_cache();
    }
    uart_puts("Cache END\r\n");
    memory_reserve(0x0000, 0x1000); // Spin tables for multicore boot (0x0000 - 0x1000)
    uart_puts("_start: 0x%x, _end: 0x%x\n", &_start, &_end);
    memory_reserve((size_t)&_start, (size_t)&_end);
    uart_puts("CPIO_START: 0x%x, CPIO_END: 0x%x\n", CPIO_START, CPIO_END);
    memory_reserve((size_t)CPIO_START, (size_t)CPIO_END);
    uart_puts("DTB_START: 0x%x, DTB_END: 0x%x\n", DTB_START, DTB_END);
    memory_reserve((size_t)DTB_START, (size_t)DTB_END);
    dump_frame();
    // while (1)
    //     ;
}

void init_cache()
{
    for (size_t i = MAX_ORDER; i >= 0; i--)
    {
        cache_freelist[i] = (list_head_t *)startup_malloc(sizeof(list_head_t));
        INIT_LIST_HEAD(cache_freelist[i]);
    }
}

/**
 * page_malloc - allocate a page frame by size
 * @size: the size of the declaration in bytes.
 * return: the address of the page frame
 */
void *page_malloc(size_t size)
{
    lock();
    int8_t val = 0;
    while (val_to_num_of_frame(val) * PAGE_FRAME_SIZE < size)
    {
        val++;
    }
    struct list_head *curr;
    size_t index = -1;
    if (!list_empty(frame_freelist[(size_t)val])) // find the exact size
    {
        curr = frame_freelist[(size_t)val]->next;
        list_del_entry(curr);
        curr->next = curr->prev = curr;
        index = frame_to_index((frame_t *)curr);
    }
    else
    {
        int8_t upper_val = val + 1;
        while (upper_val <= MAX_VAL) // find upper size of frame
        {
            if (!list_empty(frame_freelist[(size_t)upper_val]))
                break;
            upper_val++;
        }
        //
        // Split the frame
        // we want val=2, upper_val=4
        // j=4
        // 0     1     2     3     4     5     6     7     8     9     10    11    12    13    14    15
        // ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
        // │  4  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │
        // └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
        //             　
        // j=3         　                                  ↓
        // ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
        // │  4  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │  3  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │
        // └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
        //                                                  ↑↑↑↑↑  change this frame to val=3
        //
        // j=2                                             ↓
        // ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
        // │  4  │ <F> │ <F> │ <F> │  2  │ <F> │ <F> │ <F> │  3  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │
        // └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
        //                          ↑↑↑↑↑  change this frame to val=2
        //
        // finish split, get first frame and set val=2     ↓
        // ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
        // │  2  │ <F> │ <F> │ <F> │  2  │ <F> │ <F> │ <F> │  3  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │
        // └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
        //  ↑↑↑↑↑  return this frame
        //
        //

        curr = frame_freelist[(size_t)upper_val]->next;
        list_del_entry(curr);
        curr->next = curr->prev = curr;
        index = frame_to_index((frame_t *)curr);
        for (size_t j = upper_val; j > val; j--) // second half frame
        {
            frame_t *buddy = get_buddy_frame(j - 1, (frame_t *)curr);
            // uart_puts("val: %2d, split: (0x%x, 0x%x)\n", j, frame_addr_to_phy_addr((frame_t *)curr), frame_addr_to_phy_addr((frame_t *)buddy));
            buddy->val = j - 1;
            // list_del_entry((list_head_t *)buddy);
            page_insert(j - 1, buddy);
        }
    }
    if (index == -1) // can't find the frame to allocate
    {
        unlock();
        return 0;
    }
    frame_array[index].val = val;
    frame_array[index].order = NOT_CACHE;
    frame_array[index].cache_used_count = 0;
    unlock();
    return frame_addr_to_phy_addr(index_to_frame(index));
}

int page_free(void *ptr)
{
    lock();
    frame_t *curr = phy_addr_to_frame(ptr);
    int8_t val = curr->val;
    curr->val = F_FRAME_VAL;
    // int new_val = val;
    while (val < MAX_VAL)
    {
        frame_t *buddy = get_buddy_frame(val, curr);
        if (frame_to_index(buddy) >= max_frame - 1 || buddy->val != val || list_empty((list_head_t *)buddy))
        {
            break;
        }
        list_del_entry((list_head_t *)buddy); // buddy is free and be able to merge
        // uart_puts("val: %2d, merge: address(0x%x, 0x%x), frame(0x%x, 0x%x)\n", val, frame_addr_to_phy_addr((frame_t *)curr), frame_addr_to_phy_addr((frame_t *)buddy), curr, buddy);

        buddy->val = F_FRAME_VAL;
        curr->val = F_FRAME_VAL;
        curr = curr < buddy ? curr : buddy;
        curr->val = ++val;
        // val++;
    }
    curr->val = val;
    // uart_puts("curr address: 0x%x, curr->prev address: 0x%x, curr->next address: 0x%x\n", frame_addr_to_phy_addr(curr), frame_addr_to_phy_addr(curr->listhead.prev), frame_addr_to_phy_addr(curr->listhead.next));
    page_insert(val, curr);
    unlock();
    return 0;
}

void dump_frame()
{
    lock();
    for (size_t i = MAX_VAL; i >= 0; i--)
    {
        size_t count = 0;
        struct list_head *curr;
        list_for_each(curr, frame_freelist[i])
        {
            count++;
        }
        uart_puts("┌───────────────────────────────────────────────────────┐\r\n");
        uart_puts("│            val:%2d ( 0x%8x B : %3d )              │\n", i, (1 << i) * PAGE_FRAME_SIZE, count);
        uart_puts("├───────────────────────────────────────────────────────┤\n");
        list_for_each(curr, frame_freelist[i])
        {
            uart_puts("│       index:%7d, address: 0x%8x, val:%2d      │\n", frame_to_index((frame_t *)curr), frame_addr_to_phy_addr((frame_t *)curr), ((frame_t *)curr)->val);
        }
        uart_puts("└───────────────────────────────────────────────────────┘\r\n");
    }
    uart_puts("---------------------------------------------------------\r\n");
    unlock();
}

void dump_cache()
{
    lock();
    for (size_t i = MAX_ORDER; i >= 0; i--)
    {
        size_t count = 0;
        struct list_head *curr;
        list_for_each(curr, cache_freelist[i])
        {
            count++;
        }
        uart_puts("┌────────────────────────────────────────────────────────────────────────────────────┐\r\n");
        uart_puts("│                          order:%2d ( 0x%4x B : %5d )                             │\n", i, order_to_size(i), count);
        uart_puts("├────────────────────────────────────────────────────────────────────────────────────┤\n");
        count = 0;
        list_for_each(curr, cache_freelist[i])
        {
            if (count >= 10)
            {
                uart_puts("│                                       ...                                          │\r\n");
                break;
            }
            uart_puts("│      index:%8d, address: 0x%8x, frame_address: 0x%8x, order:%2d      │\n", cache_to_index(curr), curr, cache_to_frame(curr), ((frame_t *)cache_to_frame(curr))->order);
            count++;
        }
        uart_puts("└────────────────────────────────────────────────────────────────────────────────────┘\r\n");
    }
    uart_puts("---------------------------------------------------------\r\n");
    unlock();
}