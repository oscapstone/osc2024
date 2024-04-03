#ifndef __BOOTLOAD_H__
#define __BOOTLOAD_H__

#define KernelAddr 0x80000
#define BootAddr   0x60000

#include "type.h"

extern uint32_t __bss_end;
extern uint32_t __bss_size;

void reallocate();
void load_kernel();

#endif
