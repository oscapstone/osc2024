#ifndef __REBOOT_H
#define __REBOOT_H

#define PM_PASSWORD 0x5a000000
#define PM_RSTC     0x3f10001c
#define PM_WDOG      0x100024

void set(long, unsigned int);
void reset(int);
void cancel_reset();

#endif