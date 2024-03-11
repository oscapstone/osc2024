#include "mailbox.h"

void mailbox_call(volatile uint32_t mailbox[]) {
  uint32_t data = (((uint32_t)(unsigned long)mailbox) & ~0xf) | 8;
  while ((get32(MAILBOX_STATUS) & MAILBOX_FULL) != 0)
    NOP;
  set32(MAILBOX_WRITE, data);
  while ((get32(MAILBOX_STATUS) & MAILBOX_EMPTY) != 0)
    NOP;
  while (get32(MAILBOX_READ) != data)
    NOP;
}

uint32_t get_board_revision() {
  __attribute__((aligned(0x10))) volatile uint32_t mailbox[7];

  mailbox[0] = 7 * 4;  // buffer size in bytes
  mailbox[1] = REQUEST_CODE;
  // tags begin
  mailbox[2] = GET_BOARD_REVISION;  // tag identifier
  mailbox[3] = 4;  // maximum of request and response value buffer's length.
  mailbox[4] = TAG_REQUEST_CODE;
  mailbox[5] = 0;  // value buffer
  // tags end
  mailbox[6] = END_TAG;

  mailbox_call(mailbox);

  return mailbox[5];  // it should be 0xa020d3 for rpi3 b+
}
