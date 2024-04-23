#ifndef __ALLOCATOR_H
#define __ALLOCATOR_H

#define MAX_ORDER 11
#define PAGE_NUM 1024
#define MAX_OBJECT_ORDER 4
#define OBJECT_NUM 1024
#define PAGE_SIZE 4096

#define BUDDY -1
#define ALLOCATED -2

#define NULL 0

typedef struct page
{
    unsigned char *address;
    int idx, val; // idx-> page index, val->page state
    int allocated_order; // if the page is allocated, allocated_order is the block order, otherwise, its value is 0.
    int object_order; // if the page is object pool, object_oder is the object order, otherwise, its value is -1.
    struct page *pre_block, *next_block;
} page;

typedef struct free_area
{
    struct page *free_list;
    unsigned long nr_free; // the number of the free block
} free_area;

typedef struct object
{
    unsigned char *address;
    int object_size;
    struct object *left, *right;
} object;

typedef struct free_object
{
    object *free_list;
    unsigned long nr_free; // the number of the free object
} free_object;

void allocator_init();

void block_list_push(int order, page *new_block);
page *block_list_pop(int order);
void block_list_remove(int order, page *remove_block);
void *allocate_block(int need_order);
void merge_free_block(int order, page *free_block_head);

void object_list_push(unsigned char *address);
object *object_list_pop(int order);

void *simple_malloc(unsigned long long size);
void *kmalloc(unsigned long long size);
void kfree(void *ptr);

void print_free_area();
void print_free_object();

#endif