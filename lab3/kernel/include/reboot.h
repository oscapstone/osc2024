#ifndef __REBOOT_H
#define __REBOOT_H

void set(long addr, unsigned int value);
void reset(int tick);
void cancel_reset();

#endif