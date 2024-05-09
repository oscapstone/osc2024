#ifndef HEAP_H
#define HEAP_H

void heap_init();
void *simple_malloc(unsigned int size, int show_info);
void simple_memset(void *ptr, int value, unsigned int num);

#endif /* HEAP_H */