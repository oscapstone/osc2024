#pragma once

#include "hardware.h"

extern volatile unsigned int mbox[36];

int mbox_call(unsigned char ch, unsigned int *mbox);
int get_board_revision(unsigned int *mbox);
int get_arm_memory_status(unsigned int *mbox);
