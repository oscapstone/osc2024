#ifndef MM_H
#define MM_H

#include "uint.h"
#include "list.h"

#define PAGE_SIZE 0x1000
#define MAX_ORDER 16

#define FREE      0
#define ALLOCATED 1

typedef struct frame_entry {
    uint32_t order;
    uint32_t status;
} frame_entry;

typedef struct frame_entry_list_head {
    list_head_t listhead;
} frame_entry_list_head;

typedef struct chunk_slot_entry {
    uint32_t size;
    uint32_t status;
} chunk_slot_entry;

typedef struct chunk_slot_list_head {
    list_head_t listhead;
} chunk_slot_list_head;

void page_frame_allocator_init(void *start, void *end);
void page_frame_init_merge();
void *page_frame_allocation(uint32_t page_num);
void page_frame_free(void *address);

void chunk_slot_allocator_init();
void chunk_slot_listhead_init();
void *chunk_slot_allocation(uint32_t size);
void chunk_slot_free(void *address);

void memory_reserve(void *start, void *end);
void memory_init();
void *malloc(uint32_t size);
void free(void *address);

void page_frame_allocator_test();
void chunk_slot_allocator_test();

/* Utility functions */

uint32_t log2n(uint32_t x);
uint32_t address2idx(void *address);
void *idx2address(uint32_t idx);
int find_fit_chunk_slot(uint32_t size);

#endif