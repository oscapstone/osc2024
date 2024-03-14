#ifndef _MBOX_H
#define _MBOX_H

#include "uart.h"

#define MBOX_BASE (MMIO_BASE + 0xB880)
#define MBOX_READ (MBOX_BASE)
#define MBOX_STATUS (MBOX_BASE + 0x18)
#define MBOX_WRITE (MBOX_BASE + 0x20)

#define MBOX_READ_PTR ((volatile unsigned int *)MBOX_READ)
#define MBOX_STATUS_PTR ((volatile unsigned int *)MBOX_STATUS)
#define MBOX_WRITE_PTR ((volatile unsigned int *)MBOX_WRITE)

#define MBOX_EMPTY 0x40000000
#define MBOX_FULL 0x80000000

/* request codes */

// specify if this tag is a request
// for response, b31 is 1 and b30-b0 is the value length in bytes
#define TAG_REQUEST_CODE 0

// specify the message is for request
#define MBOX_REQUEST 0

/* response codes */

// request success code, it's 0x80000001 for error
#define MBOX_RESPONSE_SUCCESS 0x80000000

/* channels */

#define MBOX_CH_POWER 0
#define MBOX_CH_FB 1
#define MBOX_CH_VUART 2
#define MBOX_CH_VCHIQ 3
#define MBOX_CH_LEDS 4
#define MBOX_CH_BTNS 5
#define MBOX_CH_TOUCH 6
#define MBOX_CH_COUNT 7

// request from ARM for response by VC (CPU -> GPU)
#define MBOX_CH_PROP 8

/* tags */

// tag identifier
#define MBOX_TAG_BOARD_REVISION 0x10002
#define MBOX_TAG_GETSERIAL 0x10004
#define MBOX_TAG_GET_MEM 0x10005

// specify tag end
#define MBOX_TAG_END 0

int mbox_call(unsigned char ch);
int get_board_revision(unsigned int *vision);
int get_arm_base_memory(unsigned int *base_addr, unsigned int *mem_size);

#endif // _MBOX_H
