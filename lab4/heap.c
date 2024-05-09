#include "include/heap.h"
#include "include/cpio.h"
#include "include/dtb.h"
#include "include/types.h"
#include "include/uart.h"

extern char _heap_top;
extern char *heap_ptr;
extern startup_memory_block_t *startup_memory_block_table_start;
extern startup_memory_block_t *startup_memory_block_table_end;
extern cpio_newc_header_t *cpio_header;
extern char *cpio_end;
extern char _start;
extern char _end;

void heap_init() {
  heap_ptr = &_heap_top;
  uart_sendline("Heap pointer: 0x%p.\n", (unsigned long)heap_ptr);
}

void *simple_malloc(unsigned int size, int show_info) {
  unsigned int alignment = 8;
  size = align_size(size, alignment);
  if (heap_ptr + size > &_end) {
    uart_sendline("[Simple Malloc] Error: Out of memory.\n");
    return NULL;
  }
  char *allocated_memory = heap_ptr;
  heap_ptr += size;
  simple_memset(allocated_memory, 0, size);
  if (show_info) {
    uart_sendline("[Simple Malloc] Allocated memory at 0x%p, size %u bytes.\n",
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

unsigned int align_size(unsigned int size, unsigned int alignment) {
  return (size + alignment - 1) & ~(alignment - 1);
}

void startup_memory_block_table_add(uint64_t start, uint64_t end) {
  uint32_t size = (uint32_t)(end - start);
  if (size == 0)
    return;

  startup_memory_block_t *new_block = (startup_memory_block_t *)simple_malloc(
      sizeof(startup_memory_block_t), 0);
  if (new_block == NULL) {
    uart_sendline("[Startup Allocation] Error: Unable to allocate memory for "
                  "a new startup memory block.\n");
    return;
  }

  new_block->address = start;
  new_block->size = size;
  new_block->next = NULL;

  if (startup_memory_block_table_end == NULL) {
    startup_memory_block_table_start = new_block;
    startup_memory_block_table_end = new_block;
  } else {
    startup_memory_block_table_end->next = new_block;
    startup_memory_block_table_end = new_block;
  }
}

void startup_memory_block_table_init() {
  uart_sendline(
      "[Startup Allocation] Reserving memory for kernel from 0x%p to 0x%p.\n",
      &_start, &_end);
  startup_memory_block_table_add((uint64_t)&_start, (uint64_t)&_end);
  uart_sendline("[Startup Allocation] Reserving memory for initramfs from 0x%p "
                "to 0x%p.\n",
                cpio_header, cpio_end);
  startup_memory_block_table_add((uint64_t)cpio_header, (uint64_t)cpio_end);
  dtb_reserve_memory();
}