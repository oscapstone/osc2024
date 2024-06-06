#pragma once
#include "mmio.hpp"

#define MAILBOX_READ   ((addr_t)(MAILBOX_BASE))
#define MAILBOX_STATUS ((addr_t)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE  ((addr_t)(MAILBOX_BASE + 0x20))
#define MAILBOX_EMPTY  0x40000000
#define MAILBOX_FULL   0x80000000

#define MBOX_END_TAG            0x00000000
#define MBOX_GET_BOARD_REVISION 0x00010002
#define MBOX_GET_ARM_MEMORY     0x00010005
#define MBOX_ALLOCATE_BUFFER    0x00040001

#define MBOX_REQUEST_CODE     0x00000000
#define MBOX_REQUEST_SUCCEED  0x80000000
#define MBOX_REQUEST_FAILED   0x80000001
#define MBOX_TAG_REQUEST_CODE 0x00000000

struct __attribute__((packed)) MboxMessage {
  volatile uint32_t tag_identifier;         // tag identifier
  volatile uint32_t max_value_buffer_size;  // maximum of value buffer's size
  volatile uint32_t tag_req_resp_code;      // tag
  volatile uint32_t value_buf[];            // value buffer
};

struct __attribute__((aligned(0x10))) __attribute__((packed)) MboxBuf {
  volatile uint32_t buf_size;           // buffer size in bytes
  volatile uint32_t buf_req_resp_code;  // ?
  volatile uint32_t buf[];
};

bool mailbox_call(uint8_t ch, MboxBuf* mbox);
uint32_t mailbox_req_tag(int value_length, uint32_t tag_identifier, int idx);
uint32_t get_board_revision();
uint32_t get_arm_memory(int idx);
