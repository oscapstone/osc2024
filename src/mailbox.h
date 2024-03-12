#pragma once
#include "mmio.h"

#define MAILBOX_READ   ((addr_t)(MAILBOX_BASE))
#define MAILBOX_STATUS ((addr_t)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE  ((addr_t)(MAILBOX_BASE + 0x20))
#define MAILBOX_EMPTY  0x40000000
#define MAILBOX_FULL   0x80000000

#define GET_BOARD_REVISION 0x00010002
#define GET_ARM_MEMORY     0x00010005
#define REQUEST_CODE       0x00000000
#define REQUEST_SUCCEED    0x80000000
#define REQUEST_FAILED     0x80000001
#define TAG_REQUEST_CODE   0x00000000
#define END_TAG            0x00000000

typedef struct Message message_t;
struct __attribute__((aligned(0x10))) __attribute__((packed)) Message {
  volatile uint32_t buf_size;               // buffer size in bytes
  volatile uint32_t buf_req_resp_code;      // ?
  volatile uint32_t tag_identifier;         // tag identifier
  volatile uint32_t max_value_buffer_size;  // maximum of value buffer's size
  volatile uint32_t tag_req_resp_code;      // tag
  volatile uint32_t value_buf[];            // value buffer
};

void mailbox_call(message_t* mailbox);
uint32_t mailbox_req_tag(int value_length, uint32_t tag_identifier, int idx);
uint32_t get_board_revision();
uint32_t get_arm_memory(int idx);
