#include "uart.h"
#include "utils.h"
#include "allocator.h"

volatile unsigned char *heap_head = ((volatile unsigned char *)(0x10000000));

page page_arr[PAGE_NUM];
free_area free_area_arr[MAX_ORDER];

void allocator_init()
{
    unsigned char *cur = (unsigned char *)heap_head;
    // initialize the page array
    for (int i = 0; i < PAGE_NUM; i++)
    {
        page_arr[i].address = cur;
        page_arr[i].idx = i;
        page_arr[i].val = (i == 0 ? 0 : BUDDY);
        page_arr[i].allocated_order = 0;
        cur += 4096;
    }
    page_arr[0].pre_block = NULL;
    page_arr[0].next_block = NULL;

    // initialize the free area array
    for (int i = 0; i < 10; i++)
    {
        free_area_arr[i].free_list = NULL;
        free_area_arr[i].nr_free = 0;
    }
    free_area_arr[10].free_list = &page_arr[0];
    free_area_arr[10].nr_free = 1;
}

void block_list_push(int order, page *new_block)
{
    page *block_tail = free_area_arr[order].free_list;
    new_block->val = order;
    new_block->pre_block = NULL;
    new_block->next_block = NULL;

    if (free_area_arr[order].free_list == NULL)
        free_area_arr[order].free_list = new_block;
    else
    {
        while (block_tail->next_block != NULL)
            block_tail = block_tail->next_block;

        block_tail->next_block = new_block;
        new_block->pre_block = block_tail;
    }
    free_area_arr[order].nr_free++;
}

page *block_list_pop(int order)
{
    page *block_head = free_area_arr[order].free_list;

    free_area_arr[order].free_list = free_area_arr[order].free_list->next_block;
    if (free_area_arr[order].free_list != NULL)
        free_area_arr[order].free_list->pre_block = NULL;
    free_area_arr[order].nr_free--;

    return block_head;
}

void block_list_remove(int order, page *remove_block)
{
    if (free_area_arr[order].free_list == remove_block) // the remove block is the first node of the free list
    {
        free_area_arr[order].free_list = free_area_arr[order].free_list->next_block;
        if (free_area_arr[order].free_list != NULL)
            free_area_arr[order].free_list->pre_block = NULL;
        free_area_arr[order].nr_free--;
    }
    else
    {
        page *del = free_area_arr[order].free_list;
        while (del != remove_block)
            del = del->next_block;

        if (del->next_block != NULL)
            del->next_block->pre_block = del->pre_block;

        if (del->pre_block != NULL)
            del->pre_block->next_block = del->next_block;
        free_area_arr[order].nr_free--;
    }
}

void *allocate_block(int need_order)
{
    int order = need_order;
    while (free_area_arr[order].nr_free == 0)
        order++;

    if (order == need_order) // free arear array exists the suite order block.
    {
        page *block_head = block_list_pop(order);

        unsigned long long page_num = power(2, order);
        for (unsigned long long i = 0; i < page_num; i++) // set all the pages in this blcok to ALLOCATED.
            (block_head + i)->val = ALLOCATED;
        block_head->allocated_order = order;

        return block_head->address;
    }
    else // doesn't exist the suite order block, release redundant block.
    {
        page *block_head = block_list_pop(order);
        while (order > need_order)
        {
            unsigned long long page_num = power(2, order);
            block_list_push(order - 1, block_head + page_num / 2);
            order--;
        }

        unsigned long long page_num = power(2, order);
        for (unsigned long long i = 0; i < page_num; i++) // set all the pages in this blcok to ALLOCATED.
            (block_head + i)->val = ALLOCATED;
        block_head->allocated_order = order;

        return block_head->address;
    }
}

void merge_free_block(int order, page *free_block_head)
{
    // restore the block state.
    free_block_head->val = order;
    free_block_head->allocated_order = 0;
    unsigned long long page_num = power(2, order);
    for (unsigned long long i = 1; i < page_num; i++)
        (free_block_head + i)->val = BUDDY;

    while (1)
    {
        int buddy_idx = free_block_head->idx ^ (1 << free_block_head->val); // find the buddy.
        
        if (page_arr[buddy_idx].val == free_block_head->val) // the buddy block is free and block size is equal to the free block.
        {
            block_list_remove(page_arr[buddy_idx].val, &page_arr[buddy_idx]); // remove buddy block from free list

            if (buddy_idx < free_block_head->idx) // the buddy block is at front of the free block.
            {
                free_block_head->val = BUDDY;
                free_block_head = &page_arr[buddy_idx];
            }
            else
                page_arr[buddy_idx].val = BUDDY;

            free_block_head->val++; // increase block size
        }
        else
        {
            block_list_push(free_block_head->val, free_block_head);
            break;
        }
    }
}

void *simple_malloc(unsigned long long size)
{
    void *re_ptr = (void *)heap_head;
    heap_head += size;

    return re_ptr;
}

void *malloc(unsigned long long size)
{
    int need_order = 0;
    int need_page = size / 4096 + (size % 4096 == 0 ? 0 : 1);
    int block_size = 1;
    while (need_page > block_size)
    {
        need_order++;
        block_size *= 2;
    }

    return allocate_block(need_order);
}

void free(void *ptr)
{
    int block_head_idx = (ptr - (void *)heap_head) / 4096;

    merge_free_block(page_arr[block_head_idx].allocated_order, &page_arr[block_head_idx]);
}

void print_free_area()
{
    for (int i = 0; i < MAX_ORDER; i++)
    {
        uart_puts("free_area_arr[");
        uart_dec(i);
        uart_puts("]: ");
        uart_dec(free_area_arr[i].nr_free);
        uart_puts("\n");

        page *cur = free_area_arr[i].free_list;
        while (cur != NULL)
        {
            uart_puts("address:");
            uart_dec(cur->idx);
            uart_puts(" ");
            cur = cur->next_block;
        }
        uart_puts("\n");
    }
}
