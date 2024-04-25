#include "uart.h"
#include "utils.h"
#include "allocator.h"

#define DEBUG 0

volatile unsigned char *heap_head = ((volatile unsigned char *)(0x10000000));

int page_num = 0;
int total_object_num = 0;

unsigned long long cpio_start = 0;
unsigned long long cpio_end = 0;

page *page_arr = NULL;
object *object_arr = NULL;

free_area free_area_arr[MAX_ORDER];
free_object free_object_arr[MAX_OBJECT_ORDER];

void startup_allocate()
{
    void *startup_start = (void *)heap_head;
    page_array_init(PAGE_BASE, PAGE_END);
    object_array_init();

    memory_reserve((void *)0, (void *)0x1000);            // Spin tables for multicore boot (0x0000 - 0x1000)
    memory_reserve((void *)0x80000, (void *)0x100000);    // Kernel image in the physical memory
    memory_reserve((void *)cpio_start, (void *)cpio_end); // Initramfs
    memory_reserve(startup_start, (void *)heap_head);     // startup allocator
    
    page_allocator_init();
    object_allocator_init();
}

void page_array_init(void *start, void *end)
{
    page_num = (end - start) / PAGE_SIZE;
    page_arr = simple_malloc(page_num * sizeof(page)); // malloc page array

    unsigned char *cur = (unsigned char *)start;
    // initialize the page array
    for (int i = 0; i < page_num; i++)
    {
        page_arr[i].idx = -1;
        page_arr[i].page_idx = i;
        page_arr[i].address = cur;
        page_arr[i].val = FREE;
        page_arr[i].order_before_allocate = 0;
        page_arr[i].object_order = -1;
        page_arr[i].pre_block = NULL;
        page_arr[i].next_block = NULL;
        page_arr[i].object_address = NULL;
        cur += PAGE_SIZE;
    }
}

void object_array_init()
{
    object_arr = simple_malloc(page_num * sizeof(object)); // malloc same number of page array
    for (int i = 0; i < page_num; i++)
    {
        object_arr[i].left = NULL;
        object_arr[i].right = NULL;
    }
}

void page_allocator_init()
{
    // initialize the free area array
    for (int i = 0; i < MAX_ORDER; i++)
    {
        free_area_arr[i].free_list = NULL;
        free_area_arr[i].nr_free = 0;
    }

    for (int i = 0; i < page_num; i++) // traverse all the pages and merge the free and continous pages to the block
    {
        if (page_arr[i].val == FREE)
        {
            int j = i;
            int count = 0;
            // find the block order
            while (page_arr[j].val == FREE)
            {
                j++;
                count++;
                if (count == 1024)
                    break;
            }
            int order = -1;
            while (count > 0)
            {
                count /= 2;
                order++;
            }
            block_list_push(order, &page_arr[i]);
            // set idx and val in this block
            for (int k = i; k < j; k++)
            {
                page_arr[k].idx = k - i;
                page_arr[k].val = (k == i ? order : BUDDY);
            }
            i = j; // set i to first page of next block 
        }
    }

    /*// move heap_head to the frame that hasn't used yet.
    heap_head = (volatile unsigned char *)((((void *)heap_head - PAGE_BASE) / PAGE_SIZE + 1) * PAGE_SIZE);

    page_start_idx = ((void *)heap_head - PAGE_BASE) / PAGE_SIZE;
    page_end_idx = page_start_idx + 1024;

    // initialize the page array
    for (int i = page_start_idx; i < page_end_idx; i++)
    {
        page_arr[i].idx = i - page_start_idx;
        page_arr[i].val = (i == page_start_idx ? 10 : BUDDY);
    }
    page_arr[page_start_idx].pre_block = NULL;
    page_arr[page_start_idx].next_block = NULL;

    // initialize the free area array
    for (int i = 0; i < MAX_ORDER; i++)
    {
        free_area_arr[i].free_list = NULL;
        free_area_arr[i].nr_free = 0;
    }
    free_area_arr[10].free_list = &page_arr[page_start_idx];
    free_area_arr[10].nr_free = 1;*/
}

void object_allocator_init()
{
    int object_size = 16;
    int cur_idx = 0;
    for (int i = 0; i < MAX_OBJECT_ORDER; i++)
    {
        void *ptr = kmalloc(PAGE_SIZE); // allocate one page to object
        page_arr[(ptr - PAGE_BASE) / PAGE_SIZE].object_order = i;
        page_arr[(ptr - PAGE_BASE) / PAGE_SIZE].object_address = object_arr + cur_idx;
        int init_idx = cur_idx;
        total_object_num += PAGE_SIZE / object_size;

        // initialize object
        for (; cur_idx < total_object_num; cur_idx++)
        {
            object_arr[cur_idx].address = ptr;
            object_arr[cur_idx].object_size = object_size;
            ptr = ptr + object_size;
        }

        // initialize object array
        object_arr[init_idx].left = NULL;
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

void memory_reserve(void *start, void *end)
{
    int start_idx = (start - PAGE_BASE) / PAGE_SIZE;
    int end_idx = (end - PAGE_BASE) / PAGE_SIZE;
    for (int i = start_idx; i <= end_idx; i++)
        page_arr[i].val = ALLOCATED;
}

void block_list_push(int order, page *new_block)
{
    new_block->val = order;
    new_block->pre_block = NULL;
    new_block->next_block = NULL;

    if (free_area_arr[order].free_list == NULL) // if the list is empty, let the head pointer to the new block.
        free_area_arr[order].free_list = new_block;
    else // if the list isn't empty, insert it to the front of the list.
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
    if (free_area_arr[order].free_list == remove_block) // the remove block is the front of the free list
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

        block_head->order_before_allocate = order;

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

        block_head->order_before_allocate = order;

        return block_head->address;
    }
}

void merge_free_block(int order, page *free_block_head)
{
    // restore the block state.
    free_block_head->val = order;
    free_block_head->order_before_allocate = 0;

    unsigned long long page_num = power(2, order);
    for (unsigned long long i = 1; i < page_num; i++)
        (free_block_head + i)->val = BUDDY;

    while (1)
    {
        int buddy_page_idx = free_block_head->page_idx + (free_block_head->idx ^ (1 << free_block_head->val)) - free_block_head->idx; // find the buddy.

        if (page_arr[buddy_page_idx].val == free_block_head->val) // the buddy block is free and block size is equal to the free block.
        {
            block_list_remove(page_arr[buddy_page_idx].val, &page_arr[buddy_page_idx]); // remove buddy block from free list

#ifdef DEBUG
            uart_puts("Merge twe blocks that the order are ");
            uart_dec(free_block_head->val);
            uart_puts(" at 0x");
            uart_hex_lower_case((unsigned long long)free_block_head->address);
            uart_puts(" and at 0x");
            uart_hex_lower_case((unsigned long long)page_arr[buddy_page_idx].address);
            uart_puts("\n");
#endif

            if (buddy_page_idx < free_block_head->page_idx) // the buddy block is at front of the free block.
            {
                free_block_head->val = BUDDY;
                free_block_head = &page_arr[buddy_page_idx];
            }
            else
                page_arr[buddy_page_idx].val = BUDDY;

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
    int page_idx = (address - (unsigned char *)PAGE_BASE) / PAGE_SIZE;                  // find the page index of object pool
    int object_idx = (page_arr[page_idx].object_address - object_arr) / sizeof(object); // find the index of first object in object array in object pool
    while (object_arr[object_idx].address != address)
        object_idx++;

    int object_order = object_arr[object_idx].object_size / 16 - 1;

    object *new_object = &object_arr[object_idx];
    new_object->left = NULL;
    new_object->right = NULL;

    if (free_object_arr[object_order].free_list == NULL) // if the list is empty, then move head to the new object
        free_object_arr[object_order].free_list = new_object;
    else // if the list isn't empty, insert the object before the front of it.
    {
        new_object->right = free_object_arr[object_order].free_list;
        free_object_arr[object_order].free_list->left = new_object;
        free_object_arr[object_order].free_list = new_object;
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

    if (object_head == NULL) // If pool is empty, allocate a new page frame from the page allocator.
    {
        int cur_idx = total_object_num;
        int init_idx = cur_idx;
        int object_size = 16 * (order + 1);
        void *ptr = kmalloc(PAGE_SIZE); // allocate page to object
        page_arr[(ptr - PAGE_BASE) / PAGE_SIZE].object_order = order;
        page_arr[(ptr - PAGE_BASE) / PAGE_SIZE].object_address = object_arr + cur_idx;
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

        object_head = free_object_arr[order].free_list;
    }

    free_object_arr[order].free_list = free_object_arr[order].free_list->right;
    if (free_object_arr[order].free_list != NULL)
        free_object_arr[order].free_list->left = NULL;
    free_object_arr[order].nr_free--;

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
    int block_head_idx = (ptr - PAGE_BASE) / PAGE_SIZE;

    if (page_arr[block_head_idx].object_order == -1) // the page is not object pool.
    {
        merge_free_block(page_arr[block_head_idx].order_before_allocate, &page_arr[block_head_idx]);

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