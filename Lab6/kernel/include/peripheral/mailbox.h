#ifndef P_MAILBOX_H
#define P_MAILBOX_H

#include "peripheral/base.h"

#define MBOX_BASE (PBASE + 0xB880)

#define MBOX_READ     (MBOX_BASE)
#define MBOX_STATUS   (MBOX_BASE + 0x18)
#define MBOX_WRITE    (MBOX_BASE + 0x20)
#define MBOX_RESPONSE 0x80000000
#define MBOX_FULL     0x80000000
#define MBOX_EMPTY    0x40000000

#define MBOX_REQUEST 0

/* channels */
#define MBOX_CH_POWER 0
#define MBOX_CH_FB    1
#define MBOX_CH_VUART 2
#define MBOX_CH_VCHIQ 3
#define MBOX_CH_LEDS  4
#define MBOX_CH_BTNS  5
#define MBOX_CH_TOUCH 6
#define MBOX_CH_COUNT 7
#define MBOX_CH_PROP  8

/* tags */
#define MBOX_TAG_GET_REVISION   0x10002
#define MBOX_TAG_GET_SERIAL     0x10004
#define MBOX_TAG_GET_ARM_MEMORY 0x10005
#define MBOX_TAG_REQUEST_CODE   0
#define MBOX_TAG_LAST           0

#endif /* P_MAILBOX_H */
