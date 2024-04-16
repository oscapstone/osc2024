#ifndef _MEMORY_H
#define _MEMORY_H

#include "type.h"
#include "util.h"
#include "io.h"

extern volatile char __end;

void* simple_alloc(uint32_t);

#endif