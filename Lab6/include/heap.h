#ifndef __HEAP_H__
#define __HEAP_H__

#include <stddef.h>
#define STACK_SIZE 4096

extern int bss_end;
extern void* heap_end;

void heap_init();
void* simple_malloc(int n);

#endif