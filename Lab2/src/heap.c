#include "heap.h"

extern char *bss_end;

void* simple_malloc(int n){
  static char *heap_ptr = NULL;
  
  if(heap_ptr == NULL)
    heap_ptr = bss_end;

  void *allocated_memory = (void *)heap_ptr;
  heap_ptr += n;

  return allocated_memory;
}