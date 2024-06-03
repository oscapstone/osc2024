#ifndef _POWER_H_
#define _POWER_H_

#include "bcm2837/rpi_mmu.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC     PHYS_TO_VIRT(0x3F10001c)
#define PM_WDOG     PHYS_TO_VIRT(0x3F100024)

#endif /*_POWER_H_*/
