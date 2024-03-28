#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include "gpio.h"

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
#define MBOX_TAG_GETSERIAL      0x10004
#define MBOX_TAG_LAST           0

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MBOX_READ       ((VUI*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((VUI*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((VUI*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((VUI*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((VUI*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((VUI*)(VIDEOCORE_MBOX+0x20))
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

typedef enum {
    MBOX_POWER_MANAGEMENT = 0,
    MBOX_FRAMEBUFFER,
    MBOX_VIRTUAL_UART,
    MBOX_VCHIQ,
    MBOX_LEDS,
    MBOX_BUTTONS,
    MBOX_TOUCHSCREEN,
    MBOX_UNUSED,
    MBOX_TAGS_ARM_TO_VC,
    MBOX_TAGS_VC_TO_ARM,
} mbox_channel_type;

enum mbox_buffer_status_code {
    MBOX_REQUEST_PROCESS = 0x00000000,
    MBOX_REQUEST_SUCCEED = 0x80000000,
    MBOX_REQUEST_FAILED  = 0x80000001,
};

typedef enum {
    /* Videocore */
    MBOX_TAG_GET_FIRMWARE_VERSION = 0x1,

    /* Hardware */
    MBOX_TAG_GET_BOARD_MODEL = 0x10001,
    MBOX_TAG_GET_BOARD_REVISION,
    MBOX_TAG_GET_BOARD_MAC_ADDRESS,
    MBOX_TAG_GET_BOARD_SERIAL,
    MBOX_TAG_GET_ARM_MEMORY,
    MBOX_TAG_GET_VC_MEMORY,
    MBOX_TAG_GET_CLOCKS,
} mbox_tag_type;

#define MBOX_TAG_REQUEST_CODE 0x00000000
#define MBOX_TAG_LAST_BYTE    0x00000000

int mbox_call(unsigned char ch);
void print_hd_info();

#endif