#ifndef MM_H
#define MM_H

#include "uint.h"
#include "list.h"

#define PAGE_SIZE 0x1000
#define MAX_ORDER 16

#define FREE      0
#define ALLOCATED 1

typedef struct frame_entry {
    uint32_t order;         // 2 ^ order * 4K
    uint32_t status;        // 是否被分配
} frame_entry;

typedef struct chunk_slot_entry {
    uint32_t size;          // page chunk size, per page, per chunk size type
    uint32_t status;        // 是否被分配
} chunk_slot_entry;

typedef struct frame_entry_list_head {
    list_head_t listhead;
} frame_entry_list_head;

typedef struct chunk_slot_list_head {
    list_head_t listhead;
} chunk_slot_list_head;

void init_page_frame_allocator(void *start, void *end);
void init_page_frame_merge();
void init_page_frame_freelist();
void *page_frame_allocation(uint32_t page_num);
void page_frame_free(void *address);

void init_chunk_slot_allocator();
void init_chunk_slot_listhead();
void *chunk_slot_allocation(uint32_t size);
void chunk_slot_free(void *address);

void memory_reserve(void *start, void *end);
void memory_init();
void *malloc(uint32_t size);

void page_frame_allocator_test();
void chunk_slot_allocator_test();

/* Utility functions */

uint32_t power(uint32_t x);
uint32_t address2idx(void *address);
void *idx2address(uint32_t idx);
int find_fit_chunk_slot(uint32_t size);
uint32_t ceiling(uint32_t number);

#endif
