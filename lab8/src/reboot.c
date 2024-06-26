#include "reboot.h"

void reset(int tick) {                 // reboot after watchdog timer expire
    *PM_RSTC = PM_PASSWORD | 0x20;  // full reset
    *PM_WDOG = PM_PASSWORD | tick;  // number of watchdog tick
}

void cancel_reset() {
    *PM_RSTC = PM_PASSWORD | 0; // cancel reset
    *PM_WDOG = PM_PASSWORD | 0; // set watchdog tick to 0 to prevent reset
}