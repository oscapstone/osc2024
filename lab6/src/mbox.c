#include "mbox.h"

int mbox_call(unsigned char ch, unsigned int *mbox) {
  unsigned int r = (unsigned int)((unsigned long)mbox & ~0xF) | (ch & 0xF);
  // Wait until we can write to the mailbox
  while (*MAILBOX_REG_STATUS & MAILBOX_FULL);
  *MAILBOX_REG_WRITE = r;  // Write the request
  while (1) {
    // Wait for the response
    while (*MAILBOX_REG_STATUS & MAILBOX_EMPTY);
    if (r == *MAILBOX_REG_READ) return mbox[1] == MAILBOX_RESPONSE;
  }
  return 0;
}

int get_board_revision(unsigned int *mbox) {
  mbox[0] = 7 * 4;
  mbox[1] = REQUEST_CODE;
  mbox[2] = TAGS_HARDWARE_BOARD_REVISION;
  mbox[3] = 4;
  mbox[4] = TAG_REQUEST_CODE;
  mbox[5] = 0;
  mbox[6] = END_TAG;
  return mbox_call(MAILBOX_CH_PROP, mbox);
}

int get_arm_memory_status(unsigned int *mbox) {
  mbox[0] = 8 * 4;
  mbox[1] = REQUEST_CODE;
  mbox[2] = TAGS_HARDWARE_ARM_MEM;
  mbox[3] = 8;
  mbox[4] = TAG_REQUEST_CODE;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = END_TAG;
  return mbox_call(MAILBOX_CH_PROP, mbox);
}