#ifndef MAILBOX_H
#define MAILBOX_H

#include "kernel/gpio.h"
#include "kernel/uart.h"

// https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
// Not sure how large to declare, but many length is less than 28, si I choosed 32?
extern volatile unsigned int mailbox[32];

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

// tags (ARM to VC)
#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY      0x00010005
// buffer content
#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
// tag format
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

// mailbox address and flags
#define MAILBOX_BASE    (MMIO_BASE    + 0xb880)

#define MAILBOX_READ    (MAILBOX_BASE + 0x00)
#define MAILBOX_STATUS  (MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE   (MAILBOX_BASE + 0x20)

#define MAILBOX_EMPTY   0x40000000
#define MAILBOX_FULL    0x80000000

int mailbox_call();
void get_board_revision();
void get_arm_mem();

#endif