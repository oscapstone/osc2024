#ifndef _MEM_H
#define _MEM_H

#include "util.h"
#include "io.h"
#include "mini_uart.h"
#include "type.h"

extern volatile char __kernel_end;
extern volatile uint32_t DEVTREE_CPIO_BASE;
extern volatile uint32_t DEVTREE_CPIO_END; 
extern volatile void* DEVTREE_BEGIN;
extern volatile void* DEVTREE_END;
extern volatile char __kernel_start;

#define MEM_BEGIN 0x00000000
#define MEM_END   0x3c000000
#define FRAME_SIZE 0x1000
#define MAX_SIZE  0x10000000
#define MAX_FRAME (MEM_END-MEM_BEGIN)/FRAME_SIZE
#define MAX_CHUNK_PER_FRAME 256

// For order / chunk size
#define MAX_ORDER 16
#define MAX_CHUNK_OPT 9
#define MERGED 0xff

// For status
#define FREE 0
#define ALLOCATED 1

typedef struct frame_idx {
    struct frame_idx* prev;
    uint32_t index;
    struct frame_idx* next;
} index_t;

typedef struct frame_info {
    uint8_t status;
    uint8_t order;
    index_t* id_node;
} frame_t;

typedef struct chunk_pool {
    uint8_t chunk_opt;
    uint32_t n_chunk;
    uint32_t free_chunk;
    uint8_t chunk_stat[MAX_CHUNK_PER_FRAME];
} pool_t;

void* simple_alloc(uint32_t);

void init_mem();
void insert_frame(uint32_t, uint32_t);
index_t* pop_frame(uint32_t);
void* buddy_allocate(uint32_t);
void buddy_free(void*);
void iterative_split(uint32_t);
void iterative_merge(uint32_t, uint32_t);
void check_frames();
void insert_chunk(uint32_t, uint32_t);
void* pop_chunk(uint32_t);
void slab_free(void*);
void* slab_allocate(uint32_t);
void check_chunks();
void* malloc(uint32_t);
void free(void*);

uint32_t addr2ID(uint32_t);
uint32_t id2Addr(uint32_t);

void memory_reserve(uint32_t, uint32_t);

#endif