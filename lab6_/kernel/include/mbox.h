#ifndef _MBOX_H_
#define _MBOX_H_

extern volatile unsigned int pt[64];

// Mailbox Register MMIO
// https://jsandler18.github.io/extra/mailbox.html
// include/bcm2837/rpi_mbox.h

// Mailbox Channels
// https://github.com/raspberrypi/firmware/wiki/Mailboxes
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

// Status Code from Broadcom Videocode Driver
// brcm_usrlib/dag/vmcsx/vcinclude/bcm2708_chip/arm_control.h
enum mbox_status_reg_bits {
    BCM_ARM_VC_MS_FULL  = 0x80000000,
    BCM_ARM_VC_MS_EMPTY = 0x40000000,
    BCM_ARM_VC_MS_LEVEL = 0x400000FF,
};

enum mbox_buffer_status_code {
    MBOX_REQUEST_PROCESS = 0x00000000,
    MBOX_REQUEST_SUCCEED = 0x80000000,
    MBOX_REQUEST_FAILED  = 0x80000001,
};

// Tag
// https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
// Included partition only
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

int mbox_call(mbox_channel_type, unsigned int);

#endif /*_MBOX_H_*/
