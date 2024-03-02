#ifndef _MBOX_H
#define _MBOX_H
#include "mmio.h"

extern volatile unsigned int mbox[36]; /* a properly aligned buffer */

/* channels */
#define MBOX_CH_POWER 0
#define MBOX_CH_FB 1
#define MBOX_CH_VUART 2
#define MBOX_CH_VCHIQ 3
#define MBOX_CH_LEDS 4
#define MBOX_CH_BTNS 5
#define MBOX_CH_TOUCH 6
#define MBOX_CH_COUNT 7
#define MBOX_CH_PROP 8

void get_arm_base_memory_sz();
void get_board_serial();
void get_board_revision();
int mbox_call(unsigned char ch);

#endif