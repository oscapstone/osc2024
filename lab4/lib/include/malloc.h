#ifndef _MALLOC_H
#define _MALLOC_H

void* my_malloc(int msize);
void memory_init();
void* malloc(unsigned int msize);
void memory_reserve(void *start_addr, void *end_addr);
void process_memory_reserve();

#endif