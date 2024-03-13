#ifndef _REBOOT_H
#define _REBOOT_H

void set(long addr, unsigned int value);
void reset(int tick);
void cancel_reset();

#endif
