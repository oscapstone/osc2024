#ifndef _PERIPHERALS_MBOX_H
#define _PERIPHERALS_MBOX_H

#include "peripherals/mmio.h"
/*
Mailbox is a communication mechanism between ARM and VideoCoreIV GPU

The mailbox mechanism consists of three components mailbox registers, channels,
and messages.
*/

#define MBOX_BASE (MMIO_BASE + 0xb880)
/* Regs */
#define MBOX_READ ((volatile unsigned int *)(MBOX_BASE + 0x00))
#define MBOX_STATUS ((volatile unsigned int *)(MBOX_BASE + 0x18))
#define MBOX_WRITE ((volatile unsigned int *)(MBOX_BASE + 0x20))

/* tags */
#define MBOX_TAG_LAST 0x00000000
#define MBOX_TAG_GET_BOARD_REVISION 0x00010002
#define MBOX_TAG_GET_BOARD_SERIAL 0x10004
#define MBOX_TAG_GET_ARM_MEM 0x10005
#define MBOX_TAG_GET_VC_MEMORY 0x00010006
#define MBOX_TAG_SET_POWER 0x28001
#define MBOX_TAG_SET_CLOCK_RATE 0x00038002
#define MBOX_TAG_SET_PHY_WIDTH_HEIGHT 0x00048003
#define MBOX_TAG_SET_VTL_WIDTH_HEIGHT 0x00048004
#define MBOX_TAG_SET_VTL_OFFSET 0x00048009
#define MBOX_TAG_SET_DEPTH 0x00048005
#define MBOX_TAG_SET_PIXEL_ORDER 0x00048006
#define MBOX_TAG_ALLOCATE_BUFFER 0x00040001
#define MBOX_TAG_GET_PITCH 0x00040008

/* CODE */
#define MBOX_CODE_BUF_REQ 0x00000000
#define MBOX_CODE_BUF_RES_SUCC 0x80000000
#define MBOX_CODE_BUF_RES_FAIL 0x80000001
#define MBOX_CODE_TAG_REQ 0x00000000

/* FLAG */
#define MBOX_EMPTY 0x40000000
#define MBOX_FULL 0x80000000

/* channels */
#define MBOX_CH_POWER 0
#define MBOX_CH_FB 1
#define MBOX_CH_VUART 2
#define MBOX_CH_VCHIQ 3
#define MBOX_CH_LEDS 4
#define MBOX_CH_BTNS 5
#define MBOX_CH_TOUCH 6
#define MBOX_CH_COUNT 7
#define MBOX_CH_PROP 8

#endif