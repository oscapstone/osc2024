#include "header/reboot.h"

//addr = value
void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

void reset(int tick) {                 // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset                0x5a000000 or 0x00000020 = 0x5a000020
    set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick   0x5a000000 or 0x000003E8 = 0x5a0003E8
}

void cancel_reset() { //reset PM_RSTC and PM_WDOG
    set(PM_RSTC, PM_PASSWORD | 0);  // full reset
    set(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}