#include "mbox.h"

volatile unsigned int __attribute__((aligned(16))) mailbox[36];

int mbox_call(unsigned char ch) {
  // combine the message address (upper 28 bits) with channel number (lower 4
  // bits)
  unsigned int r =
      (((unsigned int)((unsigned long)&mailbox) & ~0xF) | (ch & 0xF));

  // wait until we can write to the mailbox
  do {
    asm volatile("nop");
  } while (*MBOX_STATUS_PTR & MBOX_FULL);

  *MBOX_WRITE_PTR = r;

  // now wait for the response
  while (1) {
    do {
      asm volatile("nop");
    } while (*MBOX_STATUS_PTR & MBOX_EMPTY);

    // check if it is a response to our message
    if (r == *MBOX_READ_PTR) {
      // check if it is a valid successful response
      // 0x80000000 means success, 0x80000001 otherwise
      return mailbox[1] == MBOX_RESPONSE_SUCCESS;
    }
  }

  return 0;
}

int get_board_revision(unsigned int *vision) {
  // buffer size in bytes
  mailbox[0] = 7 * 4;

  // buffer request/response code, it's 0 for all requests
  mailbox[1] = MBOX_REQUEST;

  // tags begin
  // tags identifier
  mailbox[2] = MBOX_TAG_BOARD_REVISION;

  // maximum of request and response value buffer's length(in bytes)
  mailbox[3] = 4;

  // tag request code, b31 clear means request
  // b30-b0 are reserved
  mailbox[4] = TAG_REQUEST_CODE;

  // value buffer
  mailbox[5] = 0;

  // 0x0 for end of tag
  mailbox[6] = MBOX_TAG_END;

  if (mbox_call(MBOX_CH_PROP) == 0) {
    return -1;
  }

  // because we value buffer is 5 above
  *vision = mailbox[5];

  return 0;
}

int get_arm_mem_info(unsigned int *base_addr, unsigned int *mem_size) {
  mailbox[0] = 8 * 4;
  mailbox[1] = MBOX_REQUEST;
  mailbox[2] = MBOX_TAG_GET_MEM;
  mailbox[3] = 8;
  mailbox[4] = TAG_REQUEST_CODE;
  mailbox[5] = 0;
  mailbox[6] = 0;
  mailbox[7] = MBOX_TAG_END;

  if (mbox_call(MBOX_CH_PROP) == 0) {
    return -1;
  }

  *base_addr = mailbox[5];
  *mem_size = mailbox[6];

  return 0;
}
