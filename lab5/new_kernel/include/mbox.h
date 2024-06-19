#ifndef _MBOX_H
#define _MBOX_H

/* a properly aligned buffer */
extern volatile unsigned int mbox[36];
#define MBOX_TAG_REQUEST_CODE 0x00000000
#define MBOX_TAG_LAST_BYTE 0x00000000
#define MBOX_REQUEST    0x00000000
typedef enum
{
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

/* channels */
#define MBOX_CH_POWER   0
// #define MBOX_CH_FB      1
// #define MBOX_CH_VUART   2
// #define MBOX_CH_VCHIQ   3
// #define MBOX_CH_LEDS    4
// #define MBOX_CH_BTNS    5
// #define MBOX_CH_TOUCH   6
// #define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

/* tags */
#define MBOX_TAG_GETSERIAL      0x10004
#define MBOX_TAG_LAST           0

// Status Code from Broadcom Videocode Driver
// brcm_usrlib/dag/vmcsx/vcinclude/bcm2708_chip/arm_control.h
enum mbox_status_reg_bits
{
    BCM_ARM_VC_MS_FULL = 0x80000000,
    BCM_ARM_VC_MS_EMPTY = 0x40000000,
    BCM_ARM_VC_MS_LEVEL = 0x400000FF,
};
enum mbox_buffer_status_code
{
    MBOX_REQUEST_PROCESS = 0x00000000,
    MBOX_REQUEST_SUCCEED = 0x80000000,
    MBOX_REQUEST_FAILED = 0x80000001,
};

typedef enum
{
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

int do_mbox_call(unsigned char ch);
int mbox_call(mbox_channel_type channel, unsigned int value);
int get_board_revision(unsigned int *board_revision);
int get_arm_memory_info(unsigned int *base_address, unsigned int *size);


#endif