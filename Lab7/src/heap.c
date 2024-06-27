#include "heap.h"

void *heap_end = (void *)0x0;
//
void heap_init(){
  unsigned long long heap_size = (unsigned long long)(8000000+(4096-((unsigned long)((char*)&bss_end+8000000)%4096))); // PAGE_SIZE
  heap_end = (void *)((char*)&bss_end+heap_size-1);
}

void* simple_malloc(int n){
  static char *heap_ptr = NULL;

  if(heap_ptr == NULL)
    heap_ptr = (char*)&bss_end;

  void *allocated_memory = (void *)heap_ptr;
  if(heap_ptr + n > (char*)heap_end)
    return NULL;
  heap_ptr += n;

  return allocated_memory;
}