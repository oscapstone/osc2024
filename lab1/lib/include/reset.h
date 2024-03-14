#ifndef __RESET_H__
#define __RESET_H__

#include "mmio.h"
#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

void reset(int tick) {                        // reboot after watchdog timer expire
    mmio_write(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    mmio_write(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
    mmio_write(PM_RSTC, PM_PASSWORD | 0);  // full reset
    mmio_write(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}

#endif