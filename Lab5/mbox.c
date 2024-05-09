/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "gpio.h"


/* mailbox message buffer */
//volatile unsigned int __attribute__((aligned(16))) mbox[36];

#define VIDEOCORE_MBOX (MMIO_BASE + 0x0000B880)
#define MBOX_READ ((volatile unsigned int *)(VIDEOCORE_MBOX + 0x0))
#define MBOX_POLL ((volatile unsigned int *)(VIDEOCORE_MBOX + 0x10))
#define MBOX_SENDER ((volatile unsigned int *)(VIDEOCORE_MBOX + 0x14))
#define MBOX_STATUS ((volatile unsigned int *)(VIDEOCORE_MBOX + 0x18))
#define MBOX_CONFIG ((volatile unsigned int *)(VIDEOCORE_MBOX + 0x1C))
#define MBOX_WRITE ((volatile unsigned int *)(VIDEOCORE_MBOX + 0x20))
#define MBOX_RESPONSE 0x80000000
#define MBOX_FULL 0x80000000
#define MBOX_EMPTY 0x40000000
#define MBOX_CH_PROP 8
/* tags */
#define GET_BOARD_REVISION 0x00010002
#define GET_ARM_MEMORY 0x00010005
#define REQUEST_CODE 0x00000000
#define REQUEST_SUCCEED 0x80000000
#define REQUEST_FAILED 0x80000001
#define TAG_REQUEST_CODE 0x00000000
#define END_TAG 0x00000000

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch, unsigned int *mbox) {
    /* 28 bits (MSB) for value, 4 bits for channel type */
    unsigned int r = (((unsigned int)((unsigned long)mbox) & ~0xF) | (ch & 0xF));
    
    /* wait until we can write to the mailbox */
    do {
        asm volatile("nop");
    } while (*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while (1) {
        /* is there a response? */
        do {
            asm volatile("nop");
        } while (*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if (r == *MBOX_READ)
            /* is it a valid successful response? */
            return mbox[1] == MBOX_RESPONSE;
    }
    return 0;
}
