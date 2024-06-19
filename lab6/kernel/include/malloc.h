#ifndef MALLOC_H
#define MALLOC_H

#include "list.h"
#include "uart.h"
#include "irq.h"
#include "dtb.h"

#define MAXORDER 6
#define MAXCACHEORDER 4 // 32, 64, 128, 256, 512  (for every 32bytes)

// simple_malloc
void *smalloc(unsigned int size);

#define BUDDYSYSTEM_START PHYS_TO_VIRT(0x0) //0x10000000L
#define BUDDYSYSTEM_PAGE_COUNT 0x3C000
//buddy system (for >= 4K pages)
void *allocpage(unsigned int size);
void freepage(void *ptr);

//Basic Exercise 2 - Dynamic Memory Allocator - 30%
//For (< 4K)
//small memory allocation
//store listhead in cache first 16 bytes
void *alloccache(unsigned int size);
void freecache(void *ptr);
void page2caches(int order);

void *malloc(unsigned int size);
void free(void *ptr);

typedef struct frame
{
    list_head_t listhead;
    int val;        // val is order
    int isused;
    int cacheorder; // -1 means isn't used for cache
    unsigned int idx;
} frame_t;

void init_allocator();
frame_t *release_redundant(frame_t *frame);
frame_t *get_buddy(frame_t *frame);
frame_t *coalesce(frame_t* f);
void dump_freelist_info();
void dump_cachelist_info();
void memory_reserve(unsigned long long start, unsigned long long end);
void alloctest();
void *memcpy(void *dest, const void *src, unsigned long long n);
#endif


