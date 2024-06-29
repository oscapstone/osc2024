#pragma once

#include <stdint.h>

#define BUDDY_MAX_ORDER 16
#define CACHE_MAX_ORDER 6
#define PAGE_SIZE 0x1000
#define MIN_CACHE_SIZE (PAGE_SIZE / (2 << CACHE_MAX_ORDER))
#define DEFAULT_STACK_SIZE 8192

struct page {
  unsigned int order;
  unsigned int used;
  unsigned int cache_order;
  struct page *prev;
  struct page *next;
};

struct object {
  unsigned int order;
  struct object *next;
};

void malloc_init();

struct page *allocatePagesByOrder(unsigned int order, int silent);
void freePages(struct page *page, unsigned int order, int silent);
void mergePages(struct page *page, unsigned int order, int echo);

void *allocateCacheMemory(unsigned int index, int silent);
void freeCacheEntry(void *ptr, unsigned int index, int silent);

void *kmalloc(unsigned int size, int silent);
void kfree(void *ptr, int silent);

void init_mem();
void reserveMemory(uint64_t start, uint64_t end);

void printFreeListByOrder(unsigned int order);

void pushPageToFreeList(struct page **list, struct page *page,
                        unsigned int order);
void removePageFromFreeList(struct page **list, struct page *page);
struct page *lookupBuddy(struct page *page, unsigned int order);