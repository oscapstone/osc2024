#include "mini_uart.h"
#include "string.h"
#include "heap.h"
#include "memory.h"

#define BASE_ADDRESS 0x0

// static frame_t *frame_array;
static frame_t *frame_array;
static frame_t *free_list[MAX_ORDER + 1];
// static AVLNode *free_list[MAX_ORDER + 1];
static kmem_cache *cache_list[MAX_SLAB_ORDER];

void list_add(int order, int index){
  frame_t *block = &frame_array[index];
  if(free_list[order] == NULL){
    free_list[order] = block;
    block->prev = block;
    block->next = block;
  }else{
    frame_t *tail = free_list[order]->prev;
    free_list[order]->prev = block;
    block->next = free_list[order];
    block->prev = tail;
    tail->next = block;
  }
}

void list_del(int order, int index){
  frame_t *block = &frame_array[index];
  if(block->prev->next != block)
    return;
  if(block->next == block){
    free_list[order] = NULL;
  }else{
    block->prev->next = block->next;
    block->next->prev = block->prev;
    if(free_list[order] == block)
      free_list[order] = block->next;
  }
}

int list_pop(int order){
  if(free_list[order] == NULL){
    return -1;
  }
  int index = free_list[order] - frame_array;
  list_del(order, index);
  return index;
}

void list_show(frame_t *head){
  frame_t *tmp = head;
  if(tmp == NULL){
    uart_sends("Empty list\n");
    return;
  }
  do{
    int index = tmp - frame_array;
    uart_sendi(index);
    uart_sends(" ");
    tmp = tmp->next;
  }while(tmp != head);
  uart_sends("\n");
}

/* split the memory block of order into 2 small blocks of (order-1) and return larger index */
int split(int index, unsigned int order){
  if(order == 0 || frame_array[index].status == MEMORY_USED || frame_array[index].order != order){
    return index;
  }

  list_del(order, index);
  int buddy_index = index ^ (1 << (order - 1));

  frame_array[index].order = order - 1;
  // frame_array[buddy_index].status = MEMORY_FREE;
  frame_array[buddy_index].order = order - 1;
  list_add(order - 1, index);
  list_add(order - 1, buddy_index);

  return buddy_index;
}

void coalesce(int index, unsigned int order){

  int buddy_index = index ^ (1 << order);
  if(frame_array[buddy_index].status != MEMORY_FREE || frame_array[buddy_index].order != order){
    return;
  }

  // frame_array[index].status = MEMORY_FREE;
  frame_array[index].order = FRAME_BODY;
  // frame_array[buddy_index].status = MEMORY_FREE;
  frame_array[buddy_index].order = FRAME_BODY;

  int merge_index = index < buddy_index ? index : buddy_index;
  unsigned int merge_order = order + 1;

  if(merge_order > MAX_ORDER){
    return;
  }
  list_del(order, index);
  list_del(order, buddy_index);

  frame_array[merge_index].order = merge_order;
  list_add(merge_order, merge_index);

  uart_sends("coalesce: ");
  uart_sendh(index*PAGE_SIZE+BASE_ADDRESS);
  uart_sends("~");
  uart_sendh(index*PAGE_SIZE+BASE_ADDRESS+(1<<order)*PAGE_SIZE);
  uart_sends(" and ");
  uart_sendh(buddy_index*PAGE_SIZE+BASE_ADDRESS);
  uart_sends("~");
  uart_sendh(buddy_index*PAGE_SIZE+BASE_ADDRESS+(1<<order)*PAGE_SIZE);
  uart_sends(" to ");
  uart_sendh(merge_index*PAGE_SIZE+BASE_ADDRESS);
  uart_sends("~");
  uart_sendh(merge_index*PAGE_SIZE+BASE_ADDRESS+(1<<merge_order)*PAGE_SIZE);
  uart_sends("\n========================\n");

  coalesce(merge_index, merge_order);
}

void buddy_init(){
  frame_array = (frame_t *)simple_malloc(FRAME_CNT * sizeof(frame_t));
  if(frame_array == NULL){
    uart_sends("Error: buddy_init failed\n");
    return;
  }
  for(int i = 0; i < FRAME_CNT; i++){
    frame_array[i].status = MEMORY_FREE;
    frame_array[i].order = FRAME_BODY;
    frame_array[i].cache_order = -1;
  }

  for(int i = 0; i <= MAX_ORDER; i++){
    free_list[i] = NULL;
  }

  int step = 1 << MAX_ORDER;
  for(int i = 0; i < FRAME_CNT; i+=step){
    frame_array[i].order = (char)MAX_ORDER;
    list_add(MAX_ORDER, i);
  }
  uart_sends("\nFinish buddy init\n");
}

void *alloc_pages(int order){
  if(order > MAX_ORDER){
    uart_sends("Error: order is too large\n");
    return NULL;
  }
  uart_sends("\nalloc page for order: ");
  uart_sendi(order);
  uart_sends("\n");

  int index = 0;
  for(int i = order; i <= MAX_ORDER; i++){
    if(free_list[i] != NULL){
      uart_sends("------------------------\nThere are free pages in order ");
      uart_sendi(i);
      uart_sends("\n");

      index = list_pop(i);
      while(i > order){
        int id = split(index, i);
          uart_sends("========================\n");
          uart_sends("split: ");
          uart_sendh(index*PAGE_SIZE+BASE_ADDRESS);
          uart_sends("~");
          uart_sendh(index*PAGE_SIZE+BASE_ADDRESS+(1<<i)*PAGE_SIZE);
          uart_sends(" to ");
          uart_sendh(index*PAGE_SIZE+BASE_ADDRESS);
          uart_sends("~");
          uart_sendh(index*PAGE_SIZE+BASE_ADDRESS+(1<<(i-1))*PAGE_SIZE);
          uart_sends(" and ");
          uart_sendh((index ^ (1 << (i - 1)))*PAGE_SIZE+BASE_ADDRESS);
          uart_sends("~");
          uart_sendh((index ^ (1 << (i - 1)))*PAGE_SIZE+BASE_ADDRESS+(1<<(i-1))*PAGE_SIZE);
  uart_sends("\n");
        index = id;
        i--;
      }
      break;
    }
    uart_sends("------------------------\nNo free page in order ");
    uart_sendi(i);
    uart_sends("\n");
  }

  frame_array[index].status = MEMORY_USED;
  // frame_array[index] ^= MEMORY_USED;
  list_del(order, index);
  uart_sends("------------------------\nallocate: ");
  uart_sendh(index*PAGE_SIZE+BASE_ADDRESS);
  uart_sends("~");
  uart_sendh(index*PAGE_SIZE+BASE_ADDRESS+(1<<frame_array[index].order)*PAGE_SIZE);
  uart_sends("\n========================\n");

  return (void *)((unsigned long)BASE_ADDRESS + index * PAGE_SIZE);
}

void free_pages(void *address){
  int index = ((unsigned long)address - BASE_ADDRESS) / PAGE_SIZE;
  if(frame_array[index].status == MEMORY_FREE){
    uart_sends("Error: free_pages failed\n");
    return;
  }
  frame_array[index].status = MEMORY_FREE;
  int order = frame_array[index].order;

  uart_sends("\nfree_pages: ");
  uart_sendh((unsigned long)address);
  uart_sends("~");
  uart_sendh((unsigned long)address+(1<<order)*PAGE_SIZE);
  uart_sends("\n------------------------\n");
  coalesce(index, order);
}


void reserve(void *start, void *end){
  uart_sends("\nReserve: ");
  uart_sendh((unsigned long)start);
  uart_sends("~");
  uart_sendh((unsigned long)end);
  uart_sends("\n");

  int start_index = ((unsigned long)start - BASE_ADDRESS) >> 12;
  int end_index = ((unsigned long)end - BASE_ADDRESS) >> 12;

  int head, tail;
  for(int step = MAX_ORDER; step > 0; step--){
    head = (start_index - start_index % (1 << (step)));
    tail = (end_index - end_index % (1 << (step)));
    if(head == tail){
      split(head, step);
    }
    else{
      for(int j = head; j <= tail; j += (1 << step)){
        split(j, step);
      }
    }
  }
  for(int i = start_index; i <= end_index; i++){
    if(frame_array[i].order == 0){
      list_del(0, i);
    }
  }
  uart_sends("------------------------\n");
}

void cache_list_add(int cache_order, kmem_cache *cache){
  if(cache_list[cache_order-1] == NULL){
    cache_list[cache_order-1] = cache;
    cache->prev = cache;
    cache->next = cache;
  }else{
    kmem_cache *tail = cache_list[cache_order-1]->prev;
    cache_list[cache_order-1]->prev = cache;
    cache->next = cache_list[cache_order-1];
    cache->prev = tail;
    tail->next = cache;
  }
}


/* Used to manage the freelist of objects(chunks) */
void object_list_add(kmem_cache *cache, object *obj){
  obj->next = cache->kmem_freelist;
  cache->kmem_freelist = obj;
}

object* object_list_pop(kmem_cache *cache){
  object *tmp = cache->kmem_freelist;
  cache->kmem_freelist = cache->kmem_freelist->next;
  return tmp;
}

kmem_cache *kmem_cache_create(int cache_order){

  if(cache_order > MAX_SLAB_ORDER){
    uart_sends("Error: kmem_cache_create failed\n");
    return NULL;
  }

  kmem_cache *cache = (kmem_cache *)simple_malloc(sizeof(kmem_cache));
  if(cache == NULL){
    uart_sends("Error: malloc failed\n");
    return NULL;
  }

  cache->cache_order = cache_order;
  cache->page_frame = alloc_pages(0);
  frame_array[((unsigned long)(cache->page_frame)-BASE_ADDRESS)/PAGE_SIZE].cache_order = cache_order;
  cache->kmem_freelist = NULL;
  int size = MIN_SLAB_SIZE << (cache_order-1);
  int num = PAGE_SIZE/size;

  for(int i = 0; i < num; i++){
    object *obj = (object *)((unsigned long)cache->page_frame + i * size);
    object_list_add(cache, obj);
  }
  cache_list_add(cache_order, cache);

  return cache;
}

void kmem_cache_init(){
  for(int i = 0; i < MAX_SLAB_ORDER; i++){
    cache_list[i] = NULL;
    kmem_cache_create(i+1);
  }
}

kmem_cache *kmem_cache_find(int size){
  int t = (size + MIN_SLAB_SIZE - 1) >> 4;

  // order = log(t)
  int order = 0;
  while(t > (1 << order))
    order++;

  if(order > MAX_SLAB_ORDER){
    uart_sends("Error: kmem_cache_find failed.\n");
    return NULL;
  }
  kmem_cache *tmp = cache_list[order];
  while(!tmp->kmem_freelist){
    tmp = tmp->next;
    if(tmp == cache_list[order]){
      uart_sends("Create a new cache.\n");
      tmp = kmem_cache_create(order);
      break;
    }
  }
  uart_sends("find a cache: order=");
  uart_sendi(tmp->cache_order);
  uart_sends(", index=");
  uart_sendi(((unsigned long)(tmp->page_frame)-BASE_ADDRESS)/PAGE_SIZE);
  uart_sends(", cache_order=");
  uart_sendi(frame_array[((unsigned long)(tmp->page_frame)-BASE_ADDRESS)/PAGE_SIZE].cache_order);
  uart_sends("\n");
  return tmp;
}

kmem_cache *address_to_cache(void *address, int index){
  unsigned long prefix = (unsigned long)address >> 12;
  int order = frame_array[index].cache_order;

  kmem_cache *tmp = cache_list[order-1];
  do{
    unsigned long page_prefix = (unsigned long)tmp->page_frame >> 12;
    if(prefix == page_prefix){
      uart_sends("address_to_cache: order=");
      uart_sendi(order);
      uart_sends(", index=");
      uart_sendi((unsigned long)(tmp->page_frame-BASE_ADDRESS)/PAGE_SIZE);
      uart_sends("\n");

      return tmp;
    }
    tmp = tmp->next;
  }while(tmp != cache_list[order]);

  return NULL;
}

void *kmem_cache_alloc(kmem_cache *cache, int size){
  object *obj = object_list_pop(cache);

  uart_sends("\nkmem_cache_alloc: ");
  uart_sendh((unsigned long)obj);
  uart_sends("~");
  uart_sendh((unsigned long)obj + (MIN_SLAB_SIZE << (cache->cache_order-1)));
  uart_sends("\n------------------------\n");


  return (void *)(obj);
}

void kmem_cache_free(kmem_cache *cache, void *address){
  object_list_add(cache, (object *)address);

  uart_sends("\nkmem_cache_free: ");
  uart_sendh((unsigned long)address);
  uart_sends("~");
  uart_sendh((unsigned long)address + (MIN_SLAB_SIZE << (cache->cache_order-1)));
  uart_sends("\n------------------------\n");
}

void *kmalloc(int size){
  if(size >= (MIN_SLAB_SIZE << (MAX_SLAB_ORDER-1))){
    int order = 0;
    while(PAGE_SIZE * (1 << order) < size){
      order++;
    }
    uart_sends("page alloc in kmalloc: order=");
    uart_sendi(order);
    uart_sends("\n");
    return alloc_pages(order);
  }
  kmem_cache *cache = kmem_cache_find(size);
  void *ptr = kmem_cache_alloc(cache, size);
  return ptr;
}

void kfree(void *address){
  int index = ((unsigned long)address - BASE_ADDRESS) / PAGE_SIZE;
  if(frame_array[index].cache_order != -1){
    kmem_cache *cache = address_to_cache(address, index);
    kmem_cache_free(cache, address);
  }
  else{
    uart_sends("free page in kfree!\n");
    free_pages(address);
  }
}

