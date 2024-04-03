#ifndef __LIB_H__
#define __LIB_H__

#include "type.h"

uint32_t strtol(const char *sptr, uint32_t base, int size);

void  mem_init();
void* simple_malloc(uint32_t size);

#endif