#pragma once

#define MBOX_REQUEST    0

/* channels */
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

/* tags */
#define TAG_REQUEST_CODE	0x00000000
#define MBOX_TAG_GETSERIAL      0x00010004
#define MBOX_TAG_GETBOARD	0x00010002
#define MBOX_TAG_GETARMMEM	0x00010005
#define MBOX_TAG_LAST           0x00000000

int mbox_call(unsigned char ch, unsigned int *mbox);