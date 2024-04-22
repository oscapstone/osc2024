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

void *kmalloc(unsigned int size)
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
    *(unsigned int *)(r - 0x8) = size;
    khtop_ptr += size;
    return r;
}

void kfree(void *ptr)
{
    // TBD
}

unsigned int memory_size;
unsigned int max_frame;
frame_t *frame_array;
list_head_t *frame_freelist[MAX_VAL];

unsigned int get_memory_size()
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
        return (unsigned int)pt[6];
    else
        return 0;
}

void memory_init()
{
    memory_size = get_memory_size();
    allocate_frame();
    // print_frame();
    void *test[10];
    test[0] = page_malloc(1 * 4096);
    print_frame();
    test[1] = page_malloc(13 * 4096);
    print_frame();
    test[2] = page_malloc(13 * 4096);
    print_frame();
    test[3] = page_malloc(50 * 4096);
    print_frame();
    test[4] = page_malloc(20 * 4096);
    print_frame();
    test[5] = page_malloc(1 * 4096);
    print_frame();
    for (int i = 5; i >= 0; i--)
    {
        page_free(test[i]);
        print_frame();
    }
}

void *startup_malloc(unsigned long long int size)
{
    // -> khtop_ptr
    //               header 0x10 bytes                   block
    // ┌──────────────────────┬────────────────┬──────────────────────┐
    // │  fill zero 0x8 bytes │ size 0x8 bytes │ size padding to 0x16 │
    // └──────────────────────┴────────────────┴──────────────────────┘
    //

    // 0x10 for heap_block header
    char *r = khtop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned long long int *)(r - 0x8) = size;
    khtop_ptr += size;
    return r;
}

int allocate_frame()
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

int allocate_frame_buddy(int begin_frame, int count, int val)
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

void *page_malloc(unsigned int size)
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
        index = (frame_t *)curr - frame_array;
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
        char buf[VSPRINT_MAX_BUF_SIZE];
        index = (frame_t *)curr - frame_array;
        char buf1[VSPRINT_MAX_BUF_SIZE];
        for (int j = upper_val; j > val; j--) // second half frame
        {
            frame_t *buddy = get_buddy(j - 1, (frame_t *)curr);
            // sprintf(buf1, "curr: %x, curr_index: %d, buddy: %x, buddy_index: %d, val: %d, j-1: %d\n", curr, (frame_t *)curr - frame_array, buddy, buddy - frame_array, frame_array[index].val, j-1);
            // uart_puts(buf1);
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
    return (void *)(index * PAGE_FRAME_SIZE);
}

int page_free(void *ptr)
{
    frame_t *curr = (frame_t *)&frame_array[(unsigned int)ptr / PAGE_FRAME_SIZE];
    int val = curr->val;
    curr->val = F_FRAME_VAL;
    // int new_val = val;
    while (val < MAX_VAL)
    {
        frame_t *buddy = get_buddy(val, curr);
        char buf[VSPRINT_MAX_BUF_SIZE];
        // sprintf(buf, "curr: %x, curr_index: %d, buddy: %x, buddy_index: %d, val: %d\n", curr, (frame_t *)curr - frame_array, buddy, buddy - frame_array, frame_array[(frame_t *)buddy - frame_array].val);
        // uart_puts(buf);
        if (buddy->val != val || list_empty((list_head_t *)buddy))
        {
            // uart_puts("b\n");
            break;
        }
        list_del_entry((list_head_t *)buddy); // buddy is free and be able to merge
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

frame_t *get_buddy(int val, frame_t *frame)
{
    // XOR(val, index)
    int index = (frame - frame_array);
    return &(frame_array[index ^ (1 << val)]);
}

int page_insert(int val, frame_t *frame)
{
    list_head_t *curr;
    list_for_each(curr, frame_freelist[val])
    {
        if ((frame_t *)curr > frame)
        {
            list_add((list_head_t *)frame, curr->prev);
            return 0;
        }
    }
    list_add_tail((list_head_t *)frame, frame_freelist[val]);
    return 0;
}

void print_frame()
{
    char buf[VSPRINT_MAX_BUF_SIZE];
    // sprintf(buf, "memory_size: %d, max_frame: %d\n", memory_size, max_frame);
    // uart_puts(buf);
    // for (int i = 0; i < max_frame; i++)
    // {
    //     if (frame_array[i].val == X_FRAME_VAL || frame_array[i].val == F_FRAME_VAL)
    //     {
    //         continue;
    //     }
    //     sprintf(buf, "index: %d, val: %d\n", i, frame_array[i].val);
    //     uart_puts(buf);
    // }
    uart_puts("------------------------------------------------------------------------------------\r\n");
    for (int i = MAX_VAL - 2; i >= 0; i--)
    {
        sprintf(buf, "---------------------- val: %d ----------------------\n", i);
        uart_puts(buf);
        struct list_head *curr;
        int count = 0;
        list_for_each(curr, frame_freelist[i])
        {
            count++;
            sprintf(buf, "address: %x, val: %d, index: %d\n", ((frame_t *)curr), ((frame_t *)curr)->val, (frame_t *)curr - &(frame_array[0]));
            uart_puts(buf);
        }
        sprintf(buf, "%d KB : %d\n", (1 << i) * 4, count);
        uart_puts(buf);
    }
    uart_puts("------------------------------------------------------------------------------------\r\n");
}