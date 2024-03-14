#ifndef __MBOX_H__
#define __MBOX_H__

#include "mmio.h"

#define VIDEOCORE_MBOX_BASE (MMIO_BASE + 0x0000B880)
#define MBOX_READ (VIDEOCORE_MBOX_BASE + 0x00)
#define MBOX_POLL (VIDEOCORE_MBOX_BASE + 0x10)
#define MBOX_SENDER (VIDEOCORE_MBOX_BASE + 0x14)
#define MBOX_STATUS (VIDEOCORE_MBOX_BASE + 0x18)
#define MBOX_CONFIG (VIDEOCORE_MBOX_BASE + 0x1C)
#define MBOX_WRITE (VIDEOCORE_MBOX_BASE + 0x20)
#define MBOX_RESPONSE (0x80000000)
#define MBOX_FULL (0x80000000)
#define MBOX_EMPTY (0x40000000)

#define MBOX_CH_POWER (0)
#define MBOX_CH_FB (1)
#define MBOX_CH_VUART (2)
#define MBOX_CH_VCHIQ (3)
#define MBOX_CH_LEDS (4)
#define MBOX_CH_BTNS (5)
#define MBOX_CH_TOUCH (6)
#define MBOX_CH_COUNT (7)
#define MBOX_CH_PROP (8)

#define MAIL_BODY_BUF_LEN (4)
#define MAIL_BUF_SIZE (MAIL_BODY_BUF_LEN << 2)
#define MAIL_PACKET_SIZE (MAIL_BUF_SIZE + 24)

#define REQUEST_CODE 0x00000000
#define REQUEST_SUCCEED 0x80000000
#define REQUEST_FAILED 0x80000001
#define TAG_REQUEST_CODE 0x00000000
#define END_TAG 0x00000000

/* Tags */
#define GET_FIRMWARE_REVISION 0x00000001
#define GET_BOARD_MODEL 0x00010001
#define GET_BOARD_REVISION 0x00010002
#define GET_BOARD_MAC_ADDR 0x00010003
#define GET_BOARD_SERIAL 0x00010004
#define GET_ARM_MEMORY 0x00010005
#define GET_VC_MEMORY 0x00010006
#define GET_CLOCKS 0x00010007

#define GET_COMMAND_LINE 0x00050001

#define GET_DMA_CHANNELS 0x00060001

#define GET_POWER_STATE 0x00020001
#define GET_TIMING 0x00020002
#define SET_POWER_STATE 0x00028001

// and so on...

/* Unique Device ID */
#define DEVICE_ID_SD_CARD 0x00000000
#define DEVICE_ID_UART0 0x00000001
#define DEVICE_ID_UART1 0x00000002
#define DEVICE_ID_USB_HCD 0x00000003
#define DEVICE_ID_I2C0 0x00000004
#define DEVICE_ID_I2C1 0x00000005
#define DEVICE_ID_I2C2 0x00000006
#define DEVICE_ID_SPI 0x00000007
#define DEVICE_ID_CCP2TX 0x00000008
#define DEVICE_ID_UNKNOWN 0x00000009
#define DEVICE_ID_UNKNOWN 0x0000000a

/* Unique Clock ID */
#define CLOCK_ID_RESERVED 0x000000000
#define CLOCK_ID_EMMC 0x000000001
#define CLOCK_ID_UART 0x000000002
#define CLOCK_ID_ARM 0x000000003
#define CLOCK_ID_CORE 0x000000004
#define CLOCK_ID_V3D 0x000000005
#define CLOCK_ID_H264 0x000000006
#define CLOCK_ID_ISP 0x000000007
#define CLOCK_ID_SDRAM 0x000000008
#define CLOCK_ID_PIXEL 0x000000009
#define CLOCK_ID_PWM 0x00000000a
#define CLOCK_ID_HEVC 0x00000000b
#define CLOCK_ID_EMMC2 0x00000000c
#define CLOCK_ID_M2MC 0x00000000d
#define CLOCK_ID_PIXEL_BVB 0x00000000e

struct mail_header {
    uint32_t packet_size;
    uint32_t code;
};

struct mail_body {
    uint32_t id;
    uint32_t buf_size;
    uint32_t code;
    uint32_t buf[MAIL_BODY_BUF_LEN];
    uint32_t end;
};

typedef struct mail_t {
    struct mail_header header;
    struct mail_body body;
} mail_t __attribute__((aligned(16)));

int mbox_call(mail_t* mbox, uint8_t ch);
void get_board_revision(uint32_t* board_revision);
void get_memory_info(uint32_t* mem_base, uint32_t* mem_size);
void get_board_serial(uint32_t* msb, uint32_t* lsb);


#endif