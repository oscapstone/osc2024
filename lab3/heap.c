#include "include/heap.h"
#include "include/types.h"
#include "include/uart.h"

extern char _heap_top;
extern char _end;
extern char *heap_ptr;

void heap_init() { heap_ptr = &_heap_top; }

void *simple_malloc(unsigned int size, int show_info) {
  unsigned int alignment = 8;
  size = (size + alignment - 1) & ~(alignment - 1);
  if (heap_ptr + size > &_end) {
    uart_sendline("Error: Out of memory.\n");
    return NULL;
  }
  char *allocated_memory = heap_ptr;
  heap_ptr += size;
  if (show_info) {
    uart_sendline("Allocated memory at: 0x%p, Size: %u bytes\n",
                  (unsigned long)allocated_memory, size);
  }
  return allocated_memory;
}

void simple_memset(void *ptr, int value, unsigned int num) {
  unsigned char *p = ptr;
  while (num--) {
    *p++ = (unsigned char)value;
  }
}