#ifndef MEMORY_H
#define MEMORY_H

extern unsigned int _end;
#define HEAP_START &_end
#define HEAP_END &_end + 0x1000
static char* HEAP_TOP = (char*)&_end;
void *simple_malloc(unsigned int size);

#define STACKSIZE   0x2000

#endif