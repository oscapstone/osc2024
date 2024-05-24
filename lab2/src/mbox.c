#include "mbox.h"

volatile unsigned int __attribute__((aligned(16))) mbox[36];

int mailbox_call(unsigned char channel) {
  unsigned int r =
      ((unsigned int)(((unsigned long)&mbox & ~0xF) | (channel & 0xF)));

  // Wait until the mailbox is not full
  while (*MAILBOX_REG_STATUS & MAILBOX_FULL) asm volatile("nop");

  // Write to the register
  *MAILBOX_REG_WRITE = r;

  while (1) {
    // Wait for the response
    while (*MAILBOX_REG_STATUS & MAILBOX_EMPTY) asm volatile("nop");

    if (r == *MAILBOX_REG_READ) return mbox[1] == TAGS_REQ_SUCCEED;
  }
  return 0;
}

int get_board_revision() {
  mbox[0] = 7 * 4;
  mbox[1] = TAGS_REQ_CODE;
  mbox[2] = TAGS_HARDWARE_BOARD_REVISION;
  mbox[3] = 4;
  mbox[4] = TAGS_REQ_CODE;
  mbox[5] = 0;
  mbox[6] = TAGS_END;
  if (mailbox_call(MAILBOX_CH_PROP)) {
    return 0;
  }
  return -1;
}

int get_arm_memory_status() {
  mbox[0] = 8 * 4;
  mbox[1] = TAGS_REQ_CODE;
  mbox[2] = TAGS_HARDWARE_ARM_MEM;
  mbox[3] = 8;
  mbox[4] = 0;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = TAGS_END;
  if (mailbox_call(MAILBOX_CH_PROP)) {
    return 0;
  }
  return -1;
}