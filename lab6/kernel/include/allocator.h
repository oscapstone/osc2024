#ifndef __ALLOCATOR_H
#define __ALLOCATOR_H

#include "mmu.h"

#define PAGE_BASE (void *)(VA_START | 0x00)
#define PAGE_END (void *)(VA_START | 0x3c000000)

#define MAX_ORDER 11
#define MAX_OBJECT_ORDER 4
#define PAGE_SIZE 4096

#define BUDDY -1
#define ALLOCATED -2
#define FREE -3

#define NULL 0

typedef struct page
{
    unsigned char *address;
    int idx, val;              // idx->index in block , val->page state
    int page_idx;              // the page index in page array
    int order_before_allocate; // if the page is allocated, the value is the block order, otherwise, its value is 0.
    int object_order;          // if the page is object pool, object_oder is the object order, otherwise, its value is -1.
    struct page *pre_block, *next_block;
    struct object *object_address; // if the page is object pool, the value is the address of the first object in object array in object pool.
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

extern unsigned long long cpio_start;
extern unsigned long long cpio_end;

void startup_allocate();
void page_array_init(void *start, void *end);
void object_array_init();
void page_allocator_init();
void object_allocator_init();

void memory_reserve(void *start, void *end);

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