#include "memory.h"
#include "mbox.h"
#include "mini_uart.h"
#include "utility.h"
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

extern unsigned char __heap_top;
extern char __begin;
extern char __end;
 char *CPIO_START = 0x80000000;
 char *CPIO_END =   0x81000000;

static unsigned char *khtop_ptr = &__heap_top;
static long memory_size;
static long max_frame;
static frame_t *frame_array;
static list_head_t *frame_freelist[MAX_VAL + 1];
static list_head_t *cache_freelist[MAX_ORDER + 1];

/*
 * val to number of frame
 */
static inline long val_to_num_of_frame(int8_t val)
{
    return 1 << val;
}

static inline long frame_to_index(frame_t *frame)
{
    return (long)((frame_t *)frame - frame_array);
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

static inline frame_t *index_to_frame(long index)
{
    return &frame_array[index];
}

static inline frame_t *cache_to_frame(void *ptr)
{
    return phy_addr_to_frame(ptr);
}

static inline long cache_to_index(void *ptr)
{
    frame_t *frame = cache_to_frame(ptr);
    return ((unsigned long)ptr - (unsigned long)frame) / (1 << (BASE_ORDER + frame->order));
}

static inline long order_to_size(int8_t order)
{
    return 1 << (BASE_ORDER + order);
}

static inline long get_buddy_index(char val, long index)
{
    // XOR(val, index)
    return index ^ val_to_num_of_frame(val);
}

static inline frame_t *get_buddy_frame(char val, frame_t *frame)
{
    // XOR(val, index)
    return &(frame_array[get_buddy_index(val, frame_to_index(frame))]);
}

static void *startup_malloc(long size)
{
    // -> khtop_ptr
    //               header 0x10 bytes                   block
    // ┌──────────────────────┬────────────────┬──────────────────────┐
    // │  fill zero 0x8 bytes │ size 0x8 bytes │ size padding to 0x16 │
    // └──────────────────────┴────────────────┴──────────────────────┘
    //
    // DEBUG("begin khtop_ptr: 0x%x\n", khtop_ptr);
    void *sp;
    asm("mov %0, sp" : "=r"(sp));
    // 0x10 for heap_block header
    unsigned char *r = khtop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned long *)(r - 0x8) = size;
    khtop_ptr += size;
    return r;
}

static void startup_free(void *ptr)
{
    // TBD
}

static int page_insert(char val, frame_t *frame)
{

    list_add((list_head_t *)frame, frame_freelist[(long)val]);
    return 0;
}

static int allocate_frame_buddy(int begin_frame, int count, char val)
{
    int size = val_to_num_of_frame(val);
    for (long i = 0; i < count; i++)
    {
        int curr_buddy = begin_frame + size * i;
        frame_array[curr_buddy].val = val;
        for (long j = 1; j < size; j++)
            frame_array[curr_buddy + j].val = F_FRAME_VAL;
        list_add_tail((list_head_t *)index_to_frame(curr_buddy), frame_freelist[(long)val]);
    }
    return 0;
}

static int allocate_frame()
{

    memory_size = 0x3B400000;

    max_frame = memory_size / PAGE_FRAME_SIZE;                                                                                                                    // each frame is 4KB
    //("memory_size: 0x%x, max_frame: 0x%x, frame_size: %d, list_head size: %d, int8_t size: %d, unsigned char size: %d\n", memory_size, max_frame, sizeof(frame_t)); // 24 Bytes
    frame_array = (frame_t *)startup_malloc(max_frame * sizeof(frame_t));                                                                                         // in 0x3C000000 Bytes Ram, array size is 0x5A0030 Bytes
    int begin_frame = 0;
    for (long i = MAX_VAL; i >= 0; i--)
    {
        frame_freelist[i] = (list_head_t *)startup_malloc(sizeof(list_head_t));
        INIT_LIST_HEAD(frame_freelist[i]);
        int count = (max_frame - begin_frame) / (1 << i);
        allocate_frame_buddy(begin_frame, count, i);
        begin_frame += count * (1 << i);
    }
    return 0;
}

void init_memory_space()
{
    allocate_frame();
    init_cache();

    memory_reserve((long)&__begin, (long)&__end);

    memory_reserve((long)CPIO_START, (long)CPIO_END);
}

void init_cache()
{
    for (long i = MAX_ORDER; i >= 0; i--)
    {
        cache_freelist[i] = (list_head_t *)startup_malloc(sizeof(list_head_t));
        INIT_LIST_HEAD(cache_freelist[i]);
    }
}

void *kmalloc(long size)
{
    lock();
    if (size >= PAGE_FRAME_SIZE / 2)
    {
        void *ptr = page_malloc(size);
        // DEBUG("use page_malloc: ptr: 0x%x, frame->order: %d, frame->cache_used_count: %d, frame->val: %d\n", ptr, phy_addr_to_frame(ptr)->order, phy_addr_to_frame(ptr)->cache_used_count, phy_addr_to_frame(ptr)->val);
        unlock();
        return ptr;
    }
    int8_t order = 0;
    while (order_to_size(order) < size)
    {
        order++;
    }
    // DEBUG("kmalloc: size: 0x%x, order: %d, cache_size: 0x%x\n", size, order, order_to_size(order));
    struct list_head *curr;
    if (!list_empty(cache_freelist[(long)order])) // find the exact size
    {
        curr = cache_freelist[(long)order]->next;
        list_del_entry(curr);
        curr->next = curr->prev = curr;
        phy_addr_to_frame(curr)->cache_used_count++;
        // DEBUG("kmalloc: find the exact size, cache address: 0x%x, frame->cache_used_count: %d, frame->val: %d, frame address: 0x%x\n", curr, phy_addr_to_frame(curr)->cache_used_count, phy_addr_to_frame(curr)->val, phy_addr_to_frame(curr));
        unlock();
        return (void *)curr;
    }

    void *ptr = page_malloc(PAGE_FRAME_SIZE);
    frame_t *frame = phy_addr_to_frame(ptr);
    // DEBUG("kmalloc: split a new frame, ptr address: 0x%x\n", ptr);
    frame->order = order;
    frame->cache_used_count = 1;
    long order_size = order_to_size(order);
    for (long i = 1; i < PAGE_FRAME_SIZE / order_size; i++)
    {
        list_add((list_head_t *)(ptr + i * order_size), cache_freelist[(long)order]);
    }
    unlock();
    return ptr;
}

void kfree(void *ptr)
{
    lock();
    frame_t *frame = cache_to_frame(ptr);
    // DEBUG("kfree: address: 0x%x, frame->order: %d, frame->cache_used_count: %d, frame->val: %d\n", ptr, frame->order, frame->cache_used_count, frame->val);
    if (frame->order == NOT_CACHE)
    {
        // DEBUG("kfree: page_free: 0x%x\n", ptr);
        page_free(ptr);
        unlock();
        return;
    }
    frame->cache_used_count--;
    // DEBUG("kfree: cache address: 0x%x, frame->order: %d, frame->cache_used_count: %d, frame->val: %d\n", ptr, frame->order, frame->cache_used_count, frame->val);
    long order_size = order_to_size(frame->order);
    list_add((list_head_t *)ptr, cache_freelist[(long)frame->order]);
    if (frame->cache_used_count == 0)
    {
        // DEBUG("kfree: cache_used_count == 0, free frame: 0x%x\n", frame);
        // DEBUG("remove the cache with the same frame from freelist\r\n");
        for (long i = 0; i < PAGE_FRAME_SIZE / order_size; i++)
        {
            list_del_entry((list_head_t *)((void *)frame_addr_to_phy_addr(frame) + i * order_size));
        }
        frame->order = NOT_CACHE;
        page_free(ptr);
        unlock();
        return;
    }
    // DEBUG("kfree: cache_used_count != 0, add to cache_freelist: 0x%x\n", ptr);
    // DEBUG("add finish\r\n");
    unlock();
    return;
}

void block(){

}

int memory_reserve(long start, long end)
{
    long start_index = start / PAGE_FRAME_SIZE;                                    // align
    long end_index = end / PAGE_FRAME_SIZE + (end % PAGE_FRAME_SIZE == 0 ? 0 : 1); // padding
    // split the start frame to fit the start address
    frame_t *start_frame = index_to_frame(start_index);
    frame_t *end_frame = index_to_frame(end_index);
    long curr_index = start_index;
    //("start reserve: start_index: (0x%x, 0x%x), end_index: (0x%x, 0x%x)\n", start_index, frame_addr_to_phy_addr(start_frame), end_index, frame_addr_to_phy_addr(end_frame));

    while (index_to_frame(curr_index)->val == F_FRAME_VAL)
    {
        curr_index--;
    }
    while (curr_index <= end_index)
    {
        frame_t *curr_frame = index_to_frame(curr_index);
        if (list_empty((list_head_t *)curr_frame))
        {
            while (1)
                ;
        }
        if (curr_index < start_index) // split when curr_index is before start_index
        {
            //("curr_index is before start_index, curr_index: (0x%x, 0x%x), start_index: (0x%x, 0x%x)\n", curr_index, frame_addr_to_phy_addr(curr_frame), start_index, frame_addr_to_phy_addr(start_frame));
            long split_val = curr_frame->val - 1;
            long buddy_index = get_buddy_index(split_val, curr_index);
            frame_t *buddy_frame = index_to_frame(buddy_index);
            curr_frame->val = split_val;
            buddy_frame->val = split_val;
            list_del_entry((list_head_t *)curr_frame);
            page_insert(split_val, curr_frame);
            page_insert(split_val, buddy_frame);
            if (buddy_index <= start_index)
            {
                //("curr_index = buddy_index, buddy_index: (0x%x, 0x%x), buddy_index->val: %d\n", buddy_index, frame_addr_to_phy_addr(buddy_frame), buddy_frame->val);
                curr_index = buddy_index;
            }
        }
        else if (start_index <= curr_index && curr_index + val_to_num_of_frame(curr_frame->val) - 1 <= end_index) // all frame is in the range
        {
            //("reserve frame: (0x%x -> 0x%x), curr_frame->val: %d\n", frame_addr_to_phy_addr(curr_frame), frame_addr_to_phy_addr(curr_frame + val_to_num_of_frame(curr_frame->val)), curr_frame->val);
            list_del_entry((list_head_t *)curr_frame);
            curr_index += val_to_num_of_frame(curr_frame->val);
        }
        else if (start_index <= curr_index && end_index < curr_index + val_to_num_of_frame(curr_frame->val) - 1) // split when frame is over end_index
        {
            //("curr_index is over end_index, curr_index: %d, end_index: %d, curr->val: %d, curr: (0x%x -> 0x%x), end: (0x%x)\n", curr_index, end_index, curr_frame->val, frame_addr_to_phy_addr(curr_frame), frame_addr_to_phy_addr(curr_frame + val_to_num_of_frame(curr_frame->val)), frame_addr_to_phy_addr(end_frame));
            long split_val = curr_frame->val - 1;
            long buddy_index = get_buddy_index(split_val, curr_index);
            frame_t *buddy_frame = index_to_frame(buddy_index);
            curr_frame->val = split_val;
            buddy_frame->val = split_val;
            //("split: (0x%x -> 0x%x), buddy: (0x%x -> 0x%x)\n", frame_addr_to_phy_addr(curr_frame), frame_addr_to_phy_addr(curr_frame + val_to_num_of_frame(curr_frame->val)), frame_addr_to_phy_addr(buddy_frame), frame_addr_to_phy_addr(buddy_frame + val_to_num_of_frame(buddy_frame->val)));
            list_del_entry((list_head_t *)curr_frame);
            page_insert(split_val, curr_frame);
            page_insert(split_val, buddy_frame);
        }
        else
        {
            while (1)
                ;
        }
    }

    //("end reserve: (0x%x -> 0x%x)\n", frame_addr_to_phy_addr(start_frame), frame_addr_to_phy_addr(end_frame + 1));
    block();
    uart_puts("fdsafd");
    return 0;
}

long get_memory_size()
{
    return 0x3c000000;
}


/**
 * page_malloc - allocate a page frame by size
 * @size: the size of the declaration in bytes.
 * return: the address of the page frame
 */
void *page_malloc(long size)
{
    lock();
    int8_t val = 0;
    while (val_to_num_of_frame(val) * PAGE_FRAME_SIZE < size)
    {
        val++;
    }
    frame_t *frame = get_free_frame(val);
    if (frame == 1) // can't find the frame to allocate
    {
        unlock();
        return 0;
    }
    frame->val = val;
    frame->order = NOT_CACHE;
    frame->cache_used_count = 0;
    unlock();
    return frame_addr_to_phy_addr(frame);
}

frame_t *get_free_frame(int val)
{
    list_head_t *curr;
    long index = -1;
    if (!list_empty(frame_freelist[(long)val])) // find the exact size
    {
        curr = frame_freelist[(long)val]->next;
    }
    else
    {
        curr = split_frame(val);
        if(curr == 1){
            return 1;
        }
    }
    index = frame_to_index((frame_t *)curr);
    list_del_entry(curr);
    curr->next = curr->prev = curr;
    return curr;
}

/**
 * Split the frame
 * we want val=2, upper_val=4
 * j=4
 * 0     1     2     3     4     5     6     7     8     9     10    11    12    13    14    15
 * ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
 * │  4  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │
 * └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
 * 　
 * j=3         　                                  ↓
 * ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
 * │  4  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │  3  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │
 * └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
 * ↑↑↑↑↑  change this frame to val=3
 *
 * j=2                                             ↓
 * ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
 * │  4  │ <F> │ <F> │ <F> │  2  │ <F> │ <F> │ <F> │  3  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │
 * └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
 * ↑↑↑↑↑  change this frame to val=2
 *
 * finish split, get first frame and set val=2     ↓
 * ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
 * │  2  │ <F> │ <F> │ <F> │  2  │ <F> │ <F> │ <F> │  3  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │
 * └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
 * ↑↑↑↑↑  return this frame
 */
frame_t *split_frame(int8_t val)
{
    struct list_head *curr;
    long index = -1;
    int8_t upper_val = val + 1;
    while (upper_val <= MAX_VAL) // find upper size of frame
    {
        if (!list_empty(frame_freelist[(long)upper_val]))
            break;
        upper_val++;
    }
    if(upper_val > MAX_VAL){
        return 1;
    }

    curr = frame_freelist[(long)upper_val]->next;

    for (long j = upper_val; j > val; j--) // second half frame
    {
        frame_t *buddy = get_buddy_frame(j - 1, (frame_t *)curr);
        buddy->val = j - 1;
        page_insert(j - 1, buddy);
    }
    return curr;
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
