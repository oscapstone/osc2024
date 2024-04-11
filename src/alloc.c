#include "alloc.h"

#include "utli.h"

static unsigned char simple_malloc_buffer[SIMPLE_MALLOC_BUFFER_SIZE];
static unsigned long simple_malloc_offset = 0;

void *simple_malloc(unsigned int size) {
  align_inplace(&size, 8);

  if (simple_malloc_offset + size >= SIMPLE_MALLOC_BUFFER_SIZE) {
    return (void *)0;
  }
  void *ret_addr = (void *)(simple_malloc_buffer + simple_malloc_offset);
  simple_malloc_offset += size;

  return ret_addr;
}