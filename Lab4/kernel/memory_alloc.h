#ifndef _MEMORY_ALLOC_H_
#define _MEMORY_ALLOC_H_

#include "buddy_system.h"

// 0x10000000 ~ 0x1FFFFFFF
// #define FREE_MEM_BASE_ADDR  0x10000000

typedef struct _slot {
    short free;
} slot;

/*
 *  Stores the slots for each frame(4 KB).
 *  Each frame consists of:
 *  32B     ->  8 slots
 *  64B     ->  4 slots
 *  128B    ->  4 slots
 *  256B    ->  4 slots
 *  512B    ->  2 slots
 *  1024B   ->  1 slot
 */
typedef struct _mem_pool {
    // Remaining available slots for each size.
    short _32B_available;
    short _64B_available;
    short _128B_available;
    short _256B_available;
    short _512B_available;
    short _1024B_available;

    // The entire block is allocated for one single request.
    short allocate_all;

    /*
     *  slot index within the frame. Calculate the physical address of the slot with the index.
     *  32B x 8 -> slot 0 ~ 7
     *  64B x 4 -> slot 8 ~ 11
     *  128B x 4 -> slot 12 ~ 15
     *  256B x 4 -> slot 16 ~ 19
     *  512B x 2 -> slot 20 ~ 21
     *  1024B x 1 -> slot 22
     */
    slot _32B[8];
    slot _64B[4];
    slot _128B[4];
    slot _256B[4];
    slot _512B[2];
    slot _1024B[1];
} mem_pool;

void init_memory_pool(void);
uint64_t dynamic_malloc(uint64_t request_size);

#endif