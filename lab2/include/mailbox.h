#ifndef __MAILBOX_H
#define __MAILBOX_H

#include "peripherals/base.h"
#include "utils.h"
#include "mini_uart.h"

#define MAILBOX_BASE        PBASE + 0xB880
#define MAILBOX_READ        MAILBOX_BASE
#define MAILBOX_WRITE       MAILBOX_BASE + 0x20
#define MAILBOX_STATUS      MAILBOX_BASE + 0x18
#define MAILBOX_RESPONSE    0x80000000
#define MAILBOX_EMPTY       0x40000000
#define MAILBOX_FULL        0x80000000
#define MAILBOX_REQUEST     0

// channels
#define MAILBOX_CH_POWER   0
#define MAILBOX_CH_FB      1
#define MAILBOX_CH_VUART   2
#define MAILBOX_CH_VCHIQ   3
#define MAILBOX_CH_LEDS    4
#define MAILBOX_CH_BTNS    5
#define MAILBOX_CH_TOUCH   6
#define MAILBOX_CH_COUNT   7
#define MAILBOX_CH_PROP    8

// tags
#define MAILBOX_TAG_GETREVISION    0x10002
#define MAILBOX_TAG_GETSERIAL      0x10004
#define MAILBOX_TAG_GETARMMEM      0x10005
#define MAILBOX_TAG_END            0


// return 0 if fail
int mailbox_call(unsigned char channel);

int get_arm_memory(unsigned int *base_addr, unsigned int *mem_size);
int get_board_serial(unsigned long long *board_serial);
int get_board_revision(unsigned int *board_revision);
void print_board_info();


#endif