#include "board/mailbox.hpp"

#include "io.hpp"
#include "mm/mmu.hpp"
#include "mm/vmm.hpp"
#include "syscall.hpp"

SYSCALL_DEFINE2(mbox_call, unsigned char, ch, MboxBuf*, mbox) {
  if (not mailbox_call(ch, mbox))
    return false;

  for (uint64_t offset = 0, size; offset < mbox->buf_size; offset += size) {
    auto idx = offset / sizeof(uint32_t);
    auto msg = (MboxMessage*)&mbox->buf[idx];
    size = sizeof(*msg) + msg->max_value_buffer_size;
    // kprintf("mbox_call %02lu: %p %x %x\n", idx, msg, msg->tag_identifier,
    //         msg->max_value_buffer_size);
    if (msg->tag_identifier == MBOX_END_TAG)
      break;
    if (msg->tag_identifier == MBOX_ALLOCATE_BUFFER) {
      auto base_addr = msg->value_buf[0] & 0x3FFFFFFF;
      auto length = msg->value_buf[1];
      klog("mbox: buf 0x%x ~ 0x%x\n", base_addr, base_addr + length);
      auto va = map_user_phy_pages(base_addr, base_addr, length, ProtFlags::RW,
                                   "[frame_buffer]");
      if (va == MAP_FAILED)
        return false;
      if (va != (uint32_t)va)
        return false;
      msg->value_buf[0] = (uint32_t)va;
    }
  }
  return true;
}

bool mailbox_call(uint8_t ch, MboxBuf* mbox) {
  auto phy_mbox = translate_va_to_pa(mbox);
  // kprintf("%p -> %p\n", mbox, phy_mbox);

  uint32_t data = (((uint32_t)(unsigned long)phy_mbox) & ~0xf) | ch;
  while ((get32(pa2va(MAILBOX_STATUS)) & MAILBOX_FULL) != 0)
    NOP;
  set32(pa2va(MAILBOX_WRITE), data);
  while ((get32(pa2va(MAILBOX_STATUS)) & MAILBOX_EMPTY) != 0)
    NOP;
  while (get32(pa2va(MAILBOX_READ)) != data)
    NOP;
  return true;
}

uint32_t mailbox_req_tag(int value_length, uint32_t tag_identifier, int idx) {
  int max_value_buffer_size = sizeof(uint32_t) * (value_length + 1);
  int size = sizeof(MboxBuf) + sizeof(MboxMessage) + max_value_buffer_size;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla-cxx-extension"
  __attribute__((aligned(0x10))) char buf[size];
#pragma GCC diagnostic pop

  auto mailbox = (MboxBuf*)buf;

  mailbox->buf_size = size;
  mailbox->buf_req_resp_code = MBOX_REQUEST_CODE;

  auto msg = (MboxMessage*)mailbox->buf;
  msg->tag_identifier = tag_identifier;
  msg->max_value_buffer_size = max_value_buffer_size;
  msg->tag_req_resp_code = MBOX_TAG_REQUEST_CODE;
  for (int i = 0; i < value_length; i++)
    msg->value_buf[i] = 0;
  msg->value_buf[value_length] = MBOX_END_TAG;

  mailbox_call(MBOX_CH_PROP, mailbox);

  return msg->value_buf[idx];
}

uint32_t get_board_revision() {
  return mailbox_req_tag(1, MBOX_GET_BOARD_REVISION, 0);
}

uint32_t get_arm_memory(int idx) {
  return mailbox_req_tag(2, MBOX_GET_ARM_MEMORY, idx);
}
