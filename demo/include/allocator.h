#ifndef ALLOCATER_H
#define ALLOCATER_H

#include "../include/my_stddef.h"
#include "../include/my_stdint.h"
#include "../include/my_stdlib.h"
#include "../include/uart.h"
#include "../include/my_string.h"

#define BUDDY_METADATA_ADDR 0x10000000
#define MAX_ORDER 11
#define MEMORY_SIZE 0x10000000  // 256 MB
#define PAGE_SIZE 4096
#define MAX_BLOCKS (MEMORY_SIZE / PAGE_SIZE)
#define NUM_POOLS 5

#define STATUS_X -1
#define STATUS_F -2
#define STATUS_D -3

extern char* __end;
extern char* _start;
extern char *cpio_addr;
extern char *dtb_addr;

typedef struct buddy_block_list_t {
    void *addr;
    int idx;
    int order;
    int size;
    struct buddy_block_list_t *next;
} buddy_block_list_t;

typedef struct buddy_system_t {
    buddy_block_list_t **buddy_list;
    int first_avail[MAX_ORDER];
    void *list_addr[MAX_ORDER];
    void *buddy_start_address;
    void *buddy_end_address;
    int max_block_amount;
    int max_index;
} buddy_system_t;

typedef struct small_block_list_t {
    void *addr;
    struct small_block_list_t *next;
} small_block_list_t;

typedef struct dynamic_memory_allocator_t {
    small_block_list_t **small_block_pools;
    //void* block_size;
} dynamic_memory_allocator_t;


void buddy_init(void);
buddy_block_list_t *get_block_addr(int order,int index);
void update_first_avail_list(void);
void* buddy_malloc(unsigned int size);
buddy_block_list_t* buddy_split(int start_index, int end_index, int req_size, int curr_order);
void mark_allocated(buddy_block_list_t* start);
void buddy_free(void *addr);
void free_child(int block_index, int order);

void dma_init(void);
void *dma_malloc(int size);
void dma_free(void *addr,int size);

void memory_reserve(void* start, void* end);
void startup_allocation(void);
void show_available_page(void);
void show_first_available_block_idx(void);

#endif