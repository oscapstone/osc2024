#ifndef _MAILBOX_H
#define _MAILBOX_H

extern volatile unsigned int __attribute__((aligned(16))) mailbox[8];

#define MAILBOX_REQ        0x00000000

/* channel */
#define MAILBOX_CH_POWER   0
#define MAILBOX_CH_FB      1
#define MAILBOX_CH_VUART   2
#define MAILBOX_CH_VCHIQ   3
#define MAILBOX_CH_LED     4
#define MAILBOX_CH_BTN     5
#define MAILBOX_TOUCH      6
#define MAILBOX_COUNT      7
#define MAILBOX_PROP       8

/* tag */
#define MAILBOX_TAG_CODE        0x00000000
#define MAILBOX_TAG_GETSERIAL   0x00010004
#define MAILBOX_TAG_GETBOARD    0x00010002
#define MAILBOX_TAG_GETARMMEM   0x00010005
#define MAILBOX_TAG_LAST        0x00000000


int mailbox_call();
void get_board_revision();
void get_arm_memory();
// void uint_to_hex(unsigned int num);


#endif