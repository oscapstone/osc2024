#pragma once

#include "hardware.h"

extern volatile unsigned int mbox[36];
int mailbox_call(unsigned char c);
int get_board_revision();
int get_arm_memory_status();
