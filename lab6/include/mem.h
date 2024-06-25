#pragma once

#include <stdint.h>

#include "start.h"

#define BUDDY_MAX_ORDER 16
#define CACHE_MAX_ORDER 6
#define MIN_CACHE_SIZE (PAGE_SIZE / (2 << CACHE_MAX_ORDER))

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

void init_mem();
void reserveMemory(uint64_t start, uint64_t end);

uintptr_t page_vaddr(struct page *p);

void pushPageToFreeList(struct page **list, struct page *page,
                        unsigned int order);
struct page *popFreeList(struct page **list_head);
void removePageFromFreeList(struct page **list, struct page *page);
void printFreeListByOrder(unsigned int order);

struct page *lookupBuddy(struct page *page, unsigned int order);
struct page *allocatePagesByOrder(unsigned int order, int silent);
void freePages(struct page *page, unsigned int order, int silent);
void mergePages(struct page *page, unsigned int order, int echo);

void pushObjectToList(struct object **list_head, struct object *object,
                      unsigned int order);
struct object *popObjectFromList(struct object **list_head);
void *allocateCacheMemory(unsigned int index, int silent);
void freeCacheEntry(void *ptr, unsigned int index, int silent);

void *kmalloc(unsigned int size, int silent);
void kfree(void *ptr, int silent);
