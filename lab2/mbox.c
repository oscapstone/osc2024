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

#include "mbox.h"
#include "uart.h"

volatile unsigned int __attribute__((aligned(16))) mbox[36];

int mbox_call(unsigned char ch) {
  unsigned int r = (((unsigned int)((unsigned long)&mbox) & ~0xF) | (ch & 0xF));

  do {
    asm volatile("nop");
  } while (*MBOX_STATUS & MBOX_FULL);
  *MBOX_WRITE = r;

  while (1) {
    do {
      asm volatile("nop");
    } while (*MBOX_STATUS & MBOX_EMPTY);
    if (r == *MBOX_READ)
      return mbox[1] == MBOX_RESPONSE;
  }

  return 0;
}

void get_board_revision() {
  mbox[0] = 8 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GETBOARD;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = MBOX_TAG_LAST;

  if (mbox_call(MBOX_CH_PROP)) {
    uart_send("Board revision: 0x%x\n", mbox[5]);
  } else {
    uart_send("Failed to get board revision.\n");
  }
}

void get_arm_memory() {
  mbox[0] = 8 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GETARMMEM;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = MBOX_TAG_LAST;

  if (mbox_call(MBOX_CH_PROP)) {
    uart_send("ARM Memory base address: 0x%x\n", mbox[5]);
    uart_send("ARM Memory size: 0x%x bytes\n", mbox[6]);
  } else {
    uart_send("Failed to get ARM memory information.\n");
  }
}
