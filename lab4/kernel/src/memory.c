#include "memory.h"
#include "mbox.h"
#include "stdio.h"
#include "uart1.h"
#include "list.h"

// Address           0k    4k    8k    12k   16k   20k   24k   28k   32k   36k   40k   44k   48k   52k   56k   60k
//                   ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
// Frame Array(val)  │  3  │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <F> │ <0> │  0  │  1  │ <F> │  2  │ <F> │ <F> │ <F> │
//                   └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
// index                0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
//
// val >=0: frame is free, size = 2^val * 4k

#define F_FRAME_VAL -1 // This frame is free, but it belongs to a larger contiguous memory block.
// #define X_FRAME_VAL -2 // This frame is already allocated.

extern char _heap_top;
static char *khtop_ptr = &_heap_top;
static unsigned int memory_size;
static unsigned int max_frame;
static frame_t *frame_array;
static list_head_t *frame_freelist[MAX_VAL];

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

static inline frame_t *index_to_frame(size_t index)
{
    return &frame_array[index];
}

static inline frame_t *get_buddy(int val, frame_t *frame)
{
    // XOR(val, index)
    return &(frame_array[frame_to_index(frame) ^ (1 << val)]);
}

static void *startup_malloc(size_t size)
{
    // -> khtop_ptr
    //               header 0x10 bytes                   block
    // ┌──────────────────────┬────────────────┬──────────────────────┐
    // │  fill zero 0x8 bytes │ size 0x8 bytes │ size padding to 0x16 │
    // └──────────────────────┴────────────────┴──────────────────────┘
    //

    void *sp;
    asm("mov %0, sp" : "=r"(sp));
    // 0x10 for heap_block header
    char *r = khtop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned long long int *)(r - 0x8) = size;
    khtop_ptr += size;
    uart_puts("khtop_ptr: 0x%x, sp: 0x%x\n", khtop_ptr, sp);
    return r;
}

static void startup_free(void *ptr)
{
    // TBD
}

static int page_insert(int val, frame_t *frame)
{
    // list_head_t *curr;
    // list_for_each(curr, frame_freelist[val])
    // {
    //     if ((frame_t *)curr > frame)
    //     {
    //         list_add((list_head_t *)frame, curr->prev);
    //         return 0;
    //     }
    // }
    // uart_puts("page_insert: frame: 0x%x, frame_freelist[%d]: 0x%x\n", frame, val,frame_freelist[val]);
    list_add((list_head_t *)frame, frame_freelist[val]);
    return 0;
}

static int allocate_frame_buddy(int begin_frame, int count, int val)
{
    int size = 1 << val;
    for (int i = 0; i < count; i++)
    {
        int curr_buddy = begin_frame + size * i;
        frame_array[curr_buddy].val = val;
        for (int j = 1; j < size; j++)
            frame_array[curr_buddy + j].val = F_FRAME_VAL;
        list_add_tail((list_head_t *)&(frame_array[curr_buddy]), frame_freelist[val]);
    }
    return 0;
}

static int allocate_frame()
{
    max_frame = memory_size / PAGE_FRAME_SIZE; // each frame is 4KB
    frame_array = (frame_t *)startup_malloc(max_frame * sizeof(frame_t));
    int begin_frame = 0;
    for (int i = MAX_VAL - 1; i >= 0; i--)
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
    // -> khtop_ptr
    //               header 0x10 bytes                   block
    // ┌──────────────────────┬────────────────┬──────────────────────┐
    // │  fill zero 0x8 bytes │ size 0x8 bytes │ size padding to 0x16 │
    // └──────────────────────┴────────────────┴──────────────────────┘

    // 0x10 for heap_block header
    char *r = khtop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(size_t *)(r - 0x8) = size;
    khtop_ptr += size;
    return r;
}

void kfree(void *ptr)
{
    // TBD
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

// dump_frame();
#define TEST_NUM 4
    int size[TEST_NUM] = {5, 2, 43, 50};
    int malloc_order[TEST_NUM] = {0, 3, 2, 1};
    void *ptr[TEST_NUM];
    int free_order[4] = {1, 3, 2, 0};
    for (int i = 0; i < TEST_NUM; i++)
    {
        ptr[i] = page_malloc(size[malloc_order[i]] * 1024);
        uart_puts("malloc: 0x%x Bytes, address: 0x%x\n", size[malloc_order[i]] * 1024, ptr[i]);
        dump_frame();
    }
    for (int i = 0; i < TEST_NUM; i++)
    {
        uart_puts("free: 0x%x Bytes, address: 0x%x\n", size[free_order[i]] * 1024, ptr[i]);
        page_free(ptr[i]);
        dump_frame();
    }
}

void *page_malloc(size_t size)
{
    int val = 0;
    while ((1 << val) * PAGE_FRAME_SIZE < size)
    {
        val++;
    }
    struct list_head *curr;
    int index = -1;
    if (!list_empty(frame_freelist[val])) // find the exact size
    {
        curr = frame_freelist[val]->next;
        list_del_entry(curr);
        index = frame_to_index((frame_t *)curr);
    }
    else
    {

        int upper_val = val + 1;
        while (upper_val < MAX_VAL) // find upper size of frame
        {
            if (!list_empty(frame_freelist[upper_val]))
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

        curr = frame_freelist[upper_val]->next;
        list_del_entry(curr);
        index = frame_to_index((frame_t *)curr);
        for (int j = upper_val; j > val; j--) // second half frame
        {
            frame_t *buddy = get_buddy(j - 1, (frame_t *)curr);
            uart_puts("val: %d, split: (0x%x, 0x%x)\n", j, frame_addr_to_phy_addr((frame_t *)curr), frame_addr_to_phy_addr((frame_t *)buddy));
            buddy->val = j - 1;
            list_del_entry((list_head_t *)buddy);
            page_insert(j - 1, buddy);
        }
    }
    if (index == -1) // can't find the frame to allocate
    {
        return 0;
    }
    frame_array[index].val = val;
    return frame_addr_to_phy_addr(index_to_frame(index));
}

int page_free(void *ptr)
{
    frame_t *curr = phy_addr_to_frame(ptr);
    int val = curr->val;
    curr->val = F_FRAME_VAL;
    // int new_val = val;
    while (val < MAX_VAL - 1)
    {
        frame_t *buddy = get_buddy(val, curr);
        if (buddy->val != val || list_empty((list_head_t *)buddy))
        {
            // uart_puts("b\n");
            break;
        }
        list_del_entry((list_head_t *)buddy); // buddy is free and be able to merge
        uart_puts("val: %d, merge: address(0x%x, 0x%x), frame(0x%x, 0x%x)\n", val, frame_addr_to_phy_addr((frame_t *)curr), frame_addr_to_phy_addr((frame_t *)buddy), curr, buddy);

        buddy->val = F_FRAME_VAL;
        curr->val = F_FRAME_VAL;
        curr = curr < buddy ? curr : buddy;
        curr->val = ++val;
        // val++;
    }
    curr->val = val;
    page_insert(val, curr);
    return 0;
}

void dump_frame()
{
    for (int i = MAX_VAL - 1; i >= 0; i--)
    {
        int count = 0;
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
}