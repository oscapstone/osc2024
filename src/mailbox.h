#pragma once
#include "mmio.h"

#define MAILBOX_READ   ((addr_t)(MAILBOX_BASE))
#define MAILBOX_STATUS ((addr_t)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE  ((addr_t)(MAILBOX_BASE + 0x20))
#define MAILBOX_EMPTY  0x40000000
#define MAILBOX_FULL   0x80000000

#define GET_BOARD_REVISION 0x00010002
#define REQUEST_CODE       0x00000000
#define REQUEST_SUCCEED    0x80000000
#define REQUEST_FAILED     0x80000001
#define TAG_REQUEST_CODE   0x00000000
#define END_TAG            0x00000000

void mailbox_call(volatile uint32_t mailbox[]);
uint32_t get_board_revision();
