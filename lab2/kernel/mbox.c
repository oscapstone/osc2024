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
#include "mbox.h"

/* mailbox message buffer */
volatile unsigned int __attribute__((aligned(16))) mbox[36];

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

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch)
{
	unsigned int r =
		(((unsigned int)((unsigned long)&mbox) & ~0xF) | (ch & 0xF));
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

void get_board_revision()
{
	mbox[0] = 7 * 4; // buffer size in bytes
	mbox[1] = MBOX_REQUEST;
	// tags begin
	mbox[2] = MBOX_TAG_GETBOARD; // tag identifier
	mbox[3] = 4; // maximum of request and response value buffer's length.
	mbox[4] = TAG_REQUEST_CODE;
	mbox[5] = 0; // value buffer
	// tags end
	mbox[6] = MBOX_TAG_LAST;
}

void get_serial_number()
{
	mbox[0] = 8 * 4; // buffer size in bytes
	mbox[1] = MBOX_REQUEST;
	// tags begin
	mbox[2] = MBOX_TAG_GETSERIAL; // tag identifier
	mbox[3] = 8; // maximum of request and response value buffer's length.
	mbox[4] = 8;
	mbox[5] = 0;
	mbox[6] = 0;
	// tags end
	mbox[7] = MBOX_TAG_LAST;
}

void get_arm_mem()
{
	mbox[0] = 8 * 4; // buffer size in bytes
	mbox[1] = MBOX_REQUEST;
	// tags begin
	mbox[2] = MBOX_TAG_GETARMMEM; // tag identifier
	mbox[3] = 8; // maximum of request and response value buffer's length.
	mbox[4] = TAG_REQUEST_CODE;
	mbox[5] = 0; // value buffer
	mbox[6] = 0; // value buffer
	// tags end
	mbox[7] = MBOX_TAG_LAST;
}
