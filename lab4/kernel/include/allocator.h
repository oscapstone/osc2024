#ifndef __ALLOCATOR_H
#define __ALLOCATOR_H

#define MAX_ORDER 11
#define PAGE_NUM 1024

#define BUDDY -1
#define ALLOCATED -2

#define NULL 0

typedef struct page
{
    unsigned char *address;
    int idx, val, allocated_order; // idx-> page index, val->page state
    struct page *pre_block, *next_block;
} page;

typedef struct free_area
{
    struct page *free_list;
    unsigned long nr_free; // the number of the free block
} free_area;

extern volatile unsigned char *heap_head;
extern page page_arr[PAGE_NUM];
extern free_area free_area_arr[MAX_ORDER];

void allocator_init();
void block_list_push(int order, page *new_block);
page *block_list_pop(int order);
void block_list_remove(int order, page* remove_block);
void *allocate_block(int need_order);
void merge_free_block(int order, page *free_block_head);

void *simple_malloc(unsigned long long size);
void *malloc(unsigned long long size);
void free(void *ptr);

void print_free_area();

#endif