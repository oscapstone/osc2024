#ifndef BCM2837BASE_H
#define BCM2837BASE_H

#include "arm/mmu.h"

//#define PBASE           0x3F000000
#define PBASE           MMU_PHYS_TO_VIRT(0x3F000000)

#define BUS_ADDR        MMU_PHYS_TO_VIRT(0x7E000000)

#define DEVICE_START    MMU_PHYS_TO_VIRT(0x3b400000)

#endif


