#ifndef __BOOTLOAD_H__
#define __BOOTLOAD_H__

#define KernelAddr 0x80000
#define BootAddr   0x60000

#include "mini_uart.h"
#include "io.h"
#include "type.h"

extern uint32_t __bss_end;

void reallocate();
void load_kernel();

#endif
