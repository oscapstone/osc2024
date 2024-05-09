#ifndef HEAP_H
#define HEAP_H

typedef struct startup_memory_block {
  struct startup_memory_block *next;
  unsigned long address;
  unsigned int size;
} startup_memory_block_t;

void heap_init();
void *simple_malloc(unsigned int size, int show_info);
void simple_memset(void *ptr, int value, unsigned int num);
unsigned int align_size(unsigned int size, unsigned int alignment);
void startup_memory_block_table_add(unsigned long start, unsigned long end);
void startup_memory_block_table_init();

#endif /* HEAP_H */