#ifndef __LIB_H__
#define __LIB_H__

#include "type.h"
#include "io.h"

uint32_t strtol(const char *sptr, uint32_t base, int size);

extern uint32_t _end;

#define HEAP_END &_end + 0x1000

void  mem_init();
void* simple_malloc(uint32_t size);

#endif