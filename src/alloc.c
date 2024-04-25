#include "alloc.h"

#include "utli.h"

static uint8_t simple_malloc_buffer[SIMPLE_MALLOC_BUFFER_SIZE];
static uint64_t simple_malloc_offset = 0;
void *startup_alloc_start = (void *)simple_malloc_buffer;
void *startup_alloc_end =
    ((void *)simple_malloc_buffer + SIMPLE_MALLOC_BUFFER_SIZE);

void *simple_malloc(uint32_t size) {
  align_inplace(&size, 8);

  if (simple_malloc_offset + size >= SIMPLE_MALLOC_BUFFER_SIZE) {
    return (void *)0;
  }
  void *ret_addr = (void *)(simple_malloc_buffer + simple_malloc_offset);
  simple_malloc_offset += size;

  return ret_addr;
}