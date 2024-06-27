#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "type.h"

uint64_t   current_exception_level();
byteptr_t  align(const byteptr_t p, uint32_t s);

int32_t    get_el();

#endif