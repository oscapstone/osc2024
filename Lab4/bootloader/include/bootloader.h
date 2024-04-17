#ifndef _BOOTLOADER_H
#define _BOOTLOADER_H

#include "type.h"
#include "io.h"

#define KERNEL_BASE (uint8_t*)(0x80000)
extern uint32_t __relocation;
extern uint32_t __end;

void load_kernel();
uint32_t get_kernel_size();

#endif