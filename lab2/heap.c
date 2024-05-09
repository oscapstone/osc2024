#include "heap.h"
#include "uart.h"
#include "utils.h"

extern char _heap_top;
extern char _end;

char *heap_ptr = &_heap_top;

void *simple_malloc(unsigned int size) {
  unsigned int alignment = 8;
  size = (size + alignment - 1) & ~(alignment - 1);

  if (heap_ptr + size > &_end) {
    return NULL;
  }

  char *allocated_memory = heap_ptr;
  heap_ptr += size;

  uart_send("Allocated memory at: 0x%p, Size: %u bytes\n",
            (unsigned long long)allocated_memory, size);

  return allocated_memory;
}