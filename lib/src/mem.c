#include "mem.h"

/* These are symbols in linker script, which don't contain value and can only be
 * used as address */
extern void _heap_start, _heap_end;
void *heap_cur = &_heap_start;

void *malloc(size_t size) {
  size = align(size, 8);

  if (heap_cur + size >= &_heap_end) {
    return (void *)0;
  }

  void *allocated = heap_cur;
  heap_cur += size;

  return allocated;
}
