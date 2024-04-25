#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "kernel/utils.h"
#include "kernel/uart.h"
#include "kernel/dtb.h"
#include "kernel/cpio.h"

#define MAX_HEAP_SIZE       8192
#define PAGE_SIZE           4096
#define BUDDY_START         0x0
#define BUDDY_END           0x3C000000
#define MAX_ORDER           18          // 2^17 ~= 128KB page frames = 512MB
#define BUDDY_METADATA_ADDR 0x10000000
// the number of memory pools
#define NUM_POOLS   6
#define MIN_POOL_SIZE 16
#define MAX_CHUNKS_PER_POOL (PAGE_SIZE / MIN_POOL_SIZE)
// Get the symbol __end from linker script
extern char* __end;
extern char* _start;
// make allocated variable global among all files
extern char* allocated;
extern int offset;

/*typedef struct buddy_block{
    unsigned int idx;           // the index of the block
    int val;                    // the order of the block
    struct buddy_block *prev;   // the previous block
    struct buddy_block *next;   // the next block
    void *addr;                 // the address of the memory block
}buddy_block_t;
// a block list for free blocks of same size
typedef struct buddy_block_list{
    buddy_block_t *block;                 // the address of the first block metadata
    int first_avail;                      // the index of the first available block
}buddy_block_list_t;*/

/*buddy->2d array with blocks metadata->usable memory*/

struct buddy_block_list{
    unsigned int idx;           // the index of the block(used minimum size block as unit, so the index is the index of the block in the whole memory block)
    int val;                    // the order of the block,-3 indicates that it's divided into smaller blocks, -2 indicate that it belongs to a larger contiguous memory block, -1 indicates that is already allocated
    struct buddy_block_list *prev;   // the previous block
    struct buddy_block_list *next;   // the next block
    int size;                   // the size of the block 
    void *addr;                 // the address of the memory block
};

typedef struct buddy_block_list buddy_block_list_t;

// a buddy system for memory allocation
struct buddy_system{
    //buddy_block_list_t *buddy_list;  // store the list of free blocks of different sizes
    buddy_block_list_t **buddy_list;
    int first_avail[MAX_ORDER];         // the index of the first available block metadata
    void* list_addr[MAX_ORDER];        // record where list starts(actually is uunecssary(as it can be calculated by adding blocks' size), just for convenience)
};

typedef struct buddy_system buddy_system_t;

extern buddy_system_t *buddy;

// return requested 'size' bytes which are continuous space
void* simple_malloc(unsigned int size);
void buddy_init(void);
void show_mem_stat(void);
void* buddy_malloc(unsigned int size);
void buddy_free(void *addr);
// a wrapper for buddy malloc, which will use memory pool if the size is smaller than a page
void* pool_alloc(unsigned int size);
void pool_free(void *ptr);
void memory_reserve(void* start,void* end);

void startup_init(void);

#endif