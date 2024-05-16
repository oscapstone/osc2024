#ifndef __MEMORY_H__
#define __MEMORY_H__

#define MAX_ORDER 10
#define FRAME_CNT 0x3c000 // 0x3c000000 / 0x1000 = 0x3c000
// #define FRAME_CNT 1024 // 0x3c000000 / 0x1000 = 0x3c000
#define PAGE_SIZE 0x1000 // 4KB

#define MEMORY_FREE (char)-1
#define MEMORY_USED (char)-2
#define FRAME_BODY (char)-1 // represent the frame is not the head of the block

#define MIN_SLAB_SIZE 16
#define MAX_SLAB_ORDER 5 // 16, 32, 64, 128, 256

// 24 bytes
typedef struct frame{
  char status;
  char order;
  int cache_order;
  struct frame *next;
  struct frame *prev;
} frame_t;

typedef struct object{
  struct object *next;
}object;


typedef struct kmem_cache{
  int cache_order;
  void *page_frame;
  // unsigned int freelist_size;
  object *kmem_freelist;
  struct kmem_cache *next;
  struct kmem_cache *prev;
} kmem_cache;


void buddy_init();
void *alloc_pages(int order);
void free_pages(void *address);

void reserve(void *start, void *end);

void kmem_cache_init();

void *kmalloc(int size);
void kfree(void *address);

#endif