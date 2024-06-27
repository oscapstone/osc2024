#include "reboot.h"
#include "c_utils.h"
#include "utils.h"

void reset(int tick) {                 // reboot after watchdog timer expire
    put32(PA2VA(PM_RSTC), PM_PASSWORD | 0x20); // full reset
    put32(PA2VA(PM_WDOG), PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
    put32(PA2VA(PM_RSTC), PM_PASSWORD | 0); // cancel reset
    put32(PA2VA(PM_WDOG), PM_PASSWORD | 0); // set watchdog tick to 0 to prevent reset
}