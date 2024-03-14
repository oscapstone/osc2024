#ifndef       _P_MAILBOX_H
#define       _P_MAILBOX_H

#include "peripherals/base.h"

#define MAILBOX_BASE MMIO_BASE + 0xb880

#define MAILBOX_READ    (unsigned int*)(MAILBOX_BASE)
#define MAILBOX_STATUS  (unsigned int*)(MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE   (unsigned int*)(MAILBOX_BASE + 0x20)

#define MAILBOX_EMPTY           0x40000000
#define MAILBOX_FULL            0x80000000

#define REQUEST_CODE            0x00000000
#define GET_BOARD_REVISION      0x00010002
#define TAG_REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED         0x80000000
#define END_TAG                 0x00000000
#define TAG_GET_ARM_MEMORY       0x00010005

#endif
 
