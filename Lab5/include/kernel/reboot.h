#ifndef REBOOT_H
#define REBOOT_H

// p.107 Clock Manager password “5a” 
#define PM_PASSWORD 0x5a000000
// Power Management Reset Control, used for controlling the system reset behavior.
#define PM_RSTC     0x3F10001c
// Power Management Watchdog?
#define PM_WDOG     0x3F100024

// write 'value' into space pointed by 'addr'
void set(long addr, unsigned int value);

void reset(int tick);

void cancel_reset();

#endif