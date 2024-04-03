#ifndef __REBOOT_H__
#define __REBOOT_H__

void set(long addr, unsigned int value);
void reset(int tick);
void cancel_reset();

#endif