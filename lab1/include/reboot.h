#ifndef _REBOOT_H
#define _REBOOT_H

#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

void set(long aadr, unsigned int value);
void reset(int tick);
void cancel_reset();

#endif
