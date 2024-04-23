#include "uart.h"
#include "utils.h"
#include "allocator.h"

#define DEBUG 0

volatile unsigned char *heap_head = ((volatile unsigned char *)(0x10000000));
int total_object_num = 0;

page page_arr[PAGE_NUM];
free_area free_area_arr[MAX_ORDER];
object object_arr[OBJECT_NUM];
free_object free_object_arr[MAX_OBJECT_ORDER];

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
        page_arr[i].object_order = -1;
        cur += PAGE_SIZE;
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

    int object_size = 16;
    int cur_idx = 0;
    for (int i = 0; i < MAX_OBJECT_ORDER; i++)
    {
        void *ptr = kmalloc(PAGE_SIZE); // allocate page to object
        page_arr[(ptr - (void *)heap_head) / PAGE_SIZE].object_order = i;
        int init_idx = cur_idx;
        total_object_num += PAGE_SIZE / object_size;

        // initialize object
        for (; cur_idx < total_object_num; cur_idx++)
        {
            object_arr[cur_idx].address = ptr;
            object_arr[cur_idx].object_size = object_size;
            object_arr[cur_idx].left = NULL;
            object_arr[cur_idx].right = NULL;
            ptr = ptr + object_size;
        }

        // initialize object array
        object_arr[init_idx].right = &object_arr[init_idx + 1];
        free_object_arr[i].free_list = &object_arr[init_idx];
        free_object_arr[i].nr_free = PAGE_SIZE / object_size;
        for (int j = init_idx + 1; j < total_object_num; j++)
        {
            object_arr[j].left = &object_arr[j - 1];
            object_arr[j].right = j + 1 < total_object_num ? &object_arr[j + 1] : NULL;
        }

        object_size += 16;
    }
}

void block_list_push(int order, page *new_block)
{
    new_block->val = order;
    new_block->pre_block = NULL;
    new_block->next_block = NULL;

    if (free_area_arr[order].free_list == NULL) // if the list is empty, let the head pointer to the new block.
        free_area_arr[order].free_list = new_block;
    else
    {
        new_block->next_block = free_area_arr[order].free_list;
        free_area_arr[order].free_list->pre_block = new_block;
        free_area_arr[order].free_list = new_block;
    }
    free_area_arr[order].nr_free++;

#ifdef DEBUG
    uart_puts("Insert the block that the order is ");
    uart_dec(order);
    uart_puts(" at 0x");
    uart_hex_lower_case((unsigned long long)(new_block->address));
    uart_puts(" to the free list.\n");
#endif
}

page *block_list_pop(int order)
{
    page *block_head = free_area_arr[order].free_list;

    if (block_head == NULL)
        return NULL;

    free_area_arr[order].free_list = free_area_arr[order].free_list->next_block; // move the head to the next block.
    if (free_area_arr[order].free_list != NULL)
        free_area_arr[order].free_list->pre_block = NULL;
    free_area_arr[order].nr_free--;

#ifdef DEBUG
    uart_puts("Remove the block that the order is ");
    uart_dec(order);
    uart_puts(" at 0x");
    uart_hex_lower_case((unsigned long long)block_head->address);
    uart_puts(" from the free list.\n");
#endif

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
        if (remove_block->next_block != NULL)
            remove_block->next_block->pre_block = remove_block->pre_block;

        if (remove_block->pre_block != NULL)
            remove_block->pre_block->next_block = remove_block->next_block;
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

#ifdef DEBUG
            uart_puts("Split the blocks that the order is ");
            uart_dec(order);
            uart_puts(" at 0x");
            uart_hex_lower_case((unsigned long long)block_head->address);
            uart_puts(" to two blocks at ");
            uart_hex_lower_case((unsigned long long)block_head->address);
            uart_puts(" and ");
            uart_hex_lower_case((unsigned long long)((block_head + page_num / 2)->address));
            uart_puts("\n");
#endif

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

#ifdef DEBUG
            uart_puts("Merge twe blocks that the order are ");
            uart_dec(free_block_head->val);
            uart_puts(" at 0x");
            uart_hex_lower_case((unsigned long long)free_block_head->address);
            uart_puts(" and at 0x");
            uart_hex_lower_case((unsigned long long)page_arr[buddy_idx].address);
            uart_puts("\n");
#endif

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

void object_list_push(unsigned char *address)
{
    int object_idx = 0;
    for (int i = 0; i < total_object_num; i++) // find the object idx in object array
        if (object_arr[i].address == address)
        {
            object_idx = i;
            break;
        }
    int object_order = object_arr[object_idx].object_size / 16 - 1;

    object *new_object = &object_arr[object_idx];
    new_object->left = NULL;
    new_object->right = NULL;

    if (free_object_arr[object_order].free_list == NULL) // if the list is empty, then move head to the new object
        free_object_arr[object_order].free_list = new_object;
    else // if the list isn't empty, insert the object after the tail of it.
    {
        object *object_tail = free_object_arr[object_order].free_list;
        while (object_tail->right != NULL)
            object_tail = object_tail->right;

        object_tail->right = new_object;
        new_object->left = object_tail;
    }
    free_object_arr[object_order].nr_free++;

#ifdef DEBUG
    uart_puts("Insert the object that the order is ");
    uart_dec(object_order);
    uart_puts(" at 0x");
    uart_hex_lower_case((unsigned long long)(new_object->address));
    uart_puts(" to the free list.\n");
#endif
}

object *object_list_pop(int order)
{
    object *object_head = free_object_arr[order].free_list;
    int object_size = 16 * (order + 1);
    if (object_head == NULL) // If pool is empty, allocate a new page frame from the page allocator.
    {
        void *ptr = kmalloc(PAGE_SIZE); // allocate page to object
        page_arr[(ptr - (void *)heap_head) / PAGE_SIZE].object_order = order;
        int cur_idx = total_object_num;
        int init_idx = cur_idx;
        total_object_num += PAGE_SIZE / object_size;

        // initialize object
        for (; cur_idx < total_object_num; cur_idx++)
        {
            object_arr[cur_idx].address = ptr;
            object_arr[cur_idx].object_size = object_size;
            object_arr[cur_idx].left = NULL;
            object_arr[cur_idx].right = NULL;
            ptr = ptr + object_size;
        }

        // initialize object array
        object_arr[init_idx].right = &object_arr[init_idx + 1];
        free_object_arr[order].free_list = &object_arr[init_idx];
        free_object_arr[order].nr_free = PAGE_SIZE / object_size;
        for (int j = init_idx + 1; j < total_object_num; j++)
        {
            object_arr[j].left = &object_arr[j - 1];
            object_arr[j].right = j + 1 < total_object_num ? &object_arr[j + 1] : NULL;
        }
    }
    else
    {
        free_object_arr[order].free_list = free_object_arr[order].free_list->right;
        if (free_object_arr[order].free_list != NULL)
            free_object_arr[order].free_list->left = NULL;
        free_object_arr[order].nr_free--;
    }

#ifdef DEBUG
    uart_puts("Remove the object that the order is ");
    uart_dec(order);
    uart_puts(" at 0x");
    uart_hex_lower_case((unsigned long long)object_head->address);
    uart_puts(" from the free list.\n");
#endif

    return object_head;
}

void *simple_malloc(unsigned long long size)
{
    void *re_ptr = (void *)heap_head;
    heap_head += size;

    return re_ptr;
}

void *kmalloc(unsigned long long size)
{
    if (size > 16 * MAX_OBJECT_ORDER)
    {
        int need_order = 0;
        int need_page = size / PAGE_SIZE + (size % PAGE_SIZE == 0 ? 0 : 1);
        int block_size = 1;
        while (need_page > block_size)
        {
            need_order++;
            block_size *= 2;
        }

        void *return_ptr = allocate_block(need_order);

#ifdef DEBUG
        print_free_area();
#endif
        return return_ptr;
    }
    else
    {
        int need_order = (size % 16 == 0) ? size / 16 - 1 : size / 16;
        object *object_ptr = object_list_pop(need_order);

#ifdef DEBUG
        print_free_object();
#endif
        return object_ptr->address;
    }
}

void kfree(void *ptr)
{
    int block_head_idx = (ptr - (void *)heap_head) / PAGE_SIZE;

    if (page_arr[block_head_idx].object_order == -1) // the page is not object pool.
    {
        merge_free_block(page_arr[block_head_idx].allocated_order, &page_arr[block_head_idx]);

#ifdef DEBUG
        print_free_area();
#endif
    }
    else
    {
        object_list_push((unsigned char *)ptr);

#ifdef DEBUG
        print_free_object();
#endif
    }
}

void print_free_area()
{
    uart_puts("-------------------------------------\n");
    for (int i = 0; i < MAX_ORDER; i++)
    {
        uart_puts("nr_free of free_area_arr[");
        uart_dec(i);
        uart_puts("]: ");
        uart_dec(free_area_arr[i].nr_free);
        uart_puts("\n");
    }
    uart_puts("-------------------------------------\n");
}

void print_free_object()
{
    uart_puts("-------------------------------------\n");
    for (int i = 0; i < MAX_OBJECT_ORDER; i++)
    {
        uart_puts("nr_free of free_object_arr[");
        uart_dec(i);
        uart_puts("]: ");
        uart_dec(free_object_arr[i].nr_free);
        uart_puts("\n");
    }
    uart_puts("-------------------------------------\n");
}