#ifndef POWER_H
#define POWER_H

#include "gpio.h"

#define PM_RSTC ((volatile unsigned int *)(MMIO_BASE + 0x0010001C))
#define PM_RSTS ((volatile unsigned int *)(MMIO_BASE + 0x00100020))
#define PM_WDOG ((volatile unsigned int *)(MMIO_BASE + 0x00100024))
#define PM_WDOG_MAGIC 0x5A000000
#define PM_RSTC_FULLRST 0x00000020

void reboot();
void abort_reboot();

#endif /* POWER_H */