#ifndef _BUDDY_SYSTEM_H_
#define _BUDDY_SYSTEM_H_

#include <stdint.h>

// 0x10000000 ~ 0x1FFFFFFF
// #define FREE_MEM_BASE_ADDR  0x10000000

// Total available memory from 0x0 to 0x3C000000.
#define FREE_MEM_BASE_ADDR 0x0
#define FREE_MEM_END_ADDR  0x3C000000

/*
 *  Total free memory size: 256 MB
 *  Memory size has to be 2's exponent, otherwise the initialization will fail.
 */
//#define FREE_MEM_SIZE   (1 << 16) * BLOCK_SIZE    
// One block -> 4KB
#define BLOCK_SIZE 4096

// 7 Linked lists -> 4, 8, 16, 32, 64, 128, 256 KB
#define FREE_BLOCK_LIST_SIZE 7

// Maximum allocation allowed.
#define MAX_ALLOC 10

// Size of free memory available.
extern uint64_t free_mem_size;

typedef struct _free_block {
    // Block index.
    int ind;
    struct _free_block* next;
    struct _free_block* prev;
} free_block;

typedef struct _list_header {
    // Exponent of current list.
    int exp;
    free_block* next_block;
} list_header;

typedef struct _allocate_info {
    // Stores the index of first frame allocated.
    int start_frame;
    // Store the number of frame pages allocated.
    int last_frame;
} allocate_info;

typedef struct _reserved_mem {
    allocate_info* info;
    struct _reserved_mem* next;
    struct _reserved_mem* prev;
} reserve_mem_list;

extern allocate_info* alloc_mem_list[MAX_ALLOC];
extern reserve_mem_list* rsv_mem_head;

void init_frame_freelist(void);
allocate_info* allocate_frame(uint64_t request_size);
uint64_t get_chunk_size(int list_ind);
int release_block(int start_ind, uint64_t chunk_size, uint64_t used_mem);
int find_chunk_exp(uint64_t size);
void check_list(void);
// After allocated frames for a request, save the info.
void insert_alloc_list(allocate_info* info);
// Free the allocated memory depending on the starting frame.
void remove_alloc_list(int start_ind);
void free_allocated_mem(int start_ind);
void show_alloc_list(void);
void merge_buddy(int ind, int free_list_ind);
void memory_reserve(uint64_t start, uint64_t end);

#endif