#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include "base.h"

#define MAILBOX_REQUEST    0

/* channels */
#define MAILBOX_CH_POWER   0
#define MAILBOX_CH_FB      1
#define MAILBOX_CH_VUART   2
#define MAILBOX_CH_VCHIQ   3
#define MAILBOX_CH_LEDS    4
#define MAILBOX_CH_BTNS    5
#define MAILBOX_CH_TOUCH   6
#define MAILBOX_CH_COUNT   7
#define MAILBOX_CH_PROP    8

/* tags */
#define TAG_REQUEST_CODE	        0x00000000
#define MAILBOX_TAG_GETBOARD	    0x00010002
#define MAILBOX_TAG_GETSERIAL       0x00010004
#define MAILBOX_TAG_GETARMMEM	    0x00010005
#define MAILBOX_TAG_END             0x00000000

#define MAILBOX_BASE                PBASE + 0xb880
#define MAILBOX_READ                ((volatile unsigned int*)MAILBOX_BASE)
#define MAILBOX_POLL                ((volatile unsigned int*)(MAILBOX_BASE + 0x10))
#define MAILBOX_SENDER              ((volatile unsigned int*)(MAILBOX_BASE + 0x14))
#define MAILBOX_STATUS              ((volatile unsigned int*)(MAILBOX_BASE + 0x18))
#define MAILBOX_CONFIG              ((volatile unsigned int*)(MAILBOX_BASE + 0x1C))
#define MAILBOX_WRITE               ((volatile unsigned int*)(MAILBOX_BASE + 0x20))
#define MAILBOX_EMPTY               0x40000000  // empty flag is represented by bit 30. Bit 30 is set(1), the mailbox is empty.
#define MAILBOX_FULL                0x80000000  // full flag is represented by bit 31. Bit 31 is set(1), the mailbox is full.
#define MAILBOX_RESPONSE_SUCCESS    0x80000000  // mailbox writes this to the message buffer if response success.
#define MAILBOX_RESPONSE_FAIL       0x80000001  // mailbox writes this to the message buffer if response fail.

void get_board_revision();
void get_arm_memory();

#endif