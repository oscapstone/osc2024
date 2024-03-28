#ifndef __MMIO_H__
#define __MMIO_H__
// #include <stddef.h>
#include "stdint.h"

#define PHY_BASE 0x3F000000
#define BUS_BASE 0x7f000000

#define MMIO_BASE PHY_BASE

void mmio_write(uint32_t reg, uint32_t data);  // MMIO write
uint32_t mmio_read(uint32_t reg);              // MMIO read

#endif