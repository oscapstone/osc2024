#include "board/mailbox.hpp"

#include "mm/mmu.hpp"
#include "syscall.hpp"

SYSCALL_DEFINE2(mbox_call, unsigned char, ch, message_t*, mbox) {
  mailbox_call(ch, mbox);
  return 1;  // TODO ???
}

void mailbox_call(uint8_t ch, message_t* mailbox) {
  uint32_t data = (((uint32_t)(unsigned long)mailbox) & ~0xf) | ch;
  while ((get32(pa2va(MAILBOX_STATUS)) & MAILBOX_FULL) != 0)
    NOP;
  set32(pa2va(MAILBOX_WRITE), data);
  while ((get32(pa2va(MAILBOX_STATUS)) & MAILBOX_EMPTY) != 0)
    NOP;
  while (get32(pa2va(MAILBOX_READ)) != data)
    NOP;
}

uint32_t mailbox_req_tag(int value_length, uint32_t tag_identifier, int idx) {
  int max_value_buffer_size = sizeof(uint32_t) * (value_length + 1);
  int size = sizeof(message_t) + max_value_buffer_size;
  __attribute__((aligned(0x10))) char buf[size];
  message_t* mailbox = (message_t*)buf;

  mailbox->buf_size = size;
  mailbox->buf_req_resp_code = REQUEST_CODE;
  mailbox->tag_identifier = tag_identifier;
  mailbox->max_value_buffer_size = max_value_buffer_size;
  mailbox->tag_req_resp_code = TAG_REQUEST_CODE;
  for (int i = 0; i < value_length; i++)
    mailbox->value_buf[i] = 0;
  mailbox->value_buf[value_length] = END_TAG;

  mailbox_call(8, mailbox);

  return mailbox->value_buf[idx];
}

uint32_t get_board_revision() {
  return mailbox_req_tag(1, GET_BOARD_REVISION, 0);
}

uint32_t get_arm_memory(int idx) {
  return mailbox_req_tag(2, GET_ARM_MEMORY, idx);
}
