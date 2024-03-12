#ifndef _DEF_MAILBOX
#define _DEF_MAILBOX

#include "uart.h"

// mailbox
#define MMIO_BASE               0x3f000000
#define MAILBOX_BASE            MMIO_BASE + 0xb880

#define MAILBOX_READ            (unsigned int*) (MAILBOX_BASE)
#define MAILBOX_STATUS          (unsigned int*) (MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE           (unsigned int*) (MAILBOX_BASE + 0x20)

#define MAILBOX_EMPTY           0x40000000
#define MAILBOX_FULL            0x80000000

#define MAILBOX_REQUEST         0x00000000
#define MAILBOX_RESPONSE        0x80000000

#define MAILBOX_BUF_REQUEST     0x00000000
#define MAILBOX_BUF_RESPONSE    0xF0000000

#define MAILBOX_CH_ARM2VC               8

#define MAILBOX_GET_BOARD_REVISION  0x00010002
#define MAILBOX_GET_ARM_MEMORY      0x00010005
#define MAILBOX_END_TAG             0x00000000

int mailbox_exec(unsigned int);
void get_board_revision(void);
void get_arm_memory(void);

#endif
