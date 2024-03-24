#include "reset.h"

void reset(int tick) {                        // reboot after watchdog timer expire
    mmio_write(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    mmio_write(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
    mmio_write(PM_RSTC, PM_PASSWORD | 0);  // full reset
    mmio_write(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}