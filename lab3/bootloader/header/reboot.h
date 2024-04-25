#ifndef _REBOOT_H_
#define _REBOOT_H_

void set(long addr, unsigned int value);
void reset(int tick);
void cancel_reset();
#endif