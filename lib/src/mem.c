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

void memset(void *src, int c, size_t n) {
  char *c_src = (char *)src;
  for (int i = 0; i < n; i++) {
    c_src[i] = c;
  }
}

void memcpy(void *dest, const void *src, size_t n) {
  const char *c_src = src;
  char *c_dest = dest;
  for (int i = 0; i < n; i++) {
    c_dest[i] = c_src[i];
  }
}
