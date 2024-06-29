#include "malloc.h"

// `__bss_end` is defined in linker script
extern char *__bss_end;

static char *heap_top;

// Set heap base address
void malloc_init() { heap_top = (char *)&__bss_end; }

void *simple_malloc(int size) {
  void *p = (void *)heap_top;
  if (size < 0) return 0;
  heap_top += size;
  return p;
}