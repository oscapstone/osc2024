#ifndef MBOX_H
#define MBOX_H

#include "gpio.h"

#define MAILBOX_BASE MMIO_BASE + 0xB880

#define MAILBOX_READ   (volatile unsigned int *)(MAILBOX_BASE)
#define MAILBOX_STATUS (volatile unsigned int *)(MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE  (volatile unsigned int *)(MAILBOX_BASE + 0x20)

#define MAILBOX_EMPTY    0x40000000
#define MAILBOX_FULL     0x80000000
#define MAILBOX_REQUEST  0x00000000
#define MAILBOX_RESPONSE 0x80000000
#define REQUEST_CODE     0x00000000
#define REQUEST_SUCCEED  0x80000000
#define REQUEST_FAILED   0x80000001
#define TAG_REQUEST_CODE 0x00000000
#define END_TAG          0x00000000

// Channels
#define MAILBOX_CH_POWER 0
#define MAILBOX_CH_FB    1
#define MAILBOX_CH_VUART 2
#define MAILBOX_CH_VCHIQ 3
#define MAILBOX_CH_LEDS  4
#define MAILBOX_CH_BTNS  5
#define MAILBOX_CH_TOUCH 6
#define MAILBOX_CH_COUNT 7
#define MAILBOX_CH_PROP  8

extern volatile unsigned int mbox[36];
int mailbox_call(unsigned char c);

#endif // MBOX_H