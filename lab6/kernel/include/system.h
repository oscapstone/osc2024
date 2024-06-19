#ifndef SYSTEM_H
#define SYSTEM_H


void set(long addr, unsigned int value);
void reset(int tick);
void cancel_reset();
void reboot();
int get_board_revision(unsigned int* board_revision);
int get_arm_memory_info(unsigned int* base_addr,unsigned int* size);
#endif