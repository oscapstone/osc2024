#include "alloc.h"

// `__bss_end` is defined in linker script
extern int __bss_end;

static char* heapTop;

// Set heap base address
void alloc_init() {
  heapTop = (char*)&__bss_end;
  heapTop++;
}

void* simple_malloc(int size) {
  void* p = (void*)heapTop;
  // If requested memory size < 0, return NULL
  if (size < 0) return 0;
  heapTop += size;
  return p;
}