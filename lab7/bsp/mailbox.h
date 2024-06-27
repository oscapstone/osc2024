#ifndef __PERIPHERALS_MAILBOX_H__
#define __PERIPHERALS_MAILBOX_H__

#define MMIO_BASE 0x3F000000  // BCM2837 ARM Peripherals Page 6
#define MAILBOX_BASE    MMIO_BASE + 0xb880

#define MAILBOX_READ    ((volatile unsigned int*)(MAILBOX_BASE       )) // M0: CPU read from GPU
#define MAILBOX_STATUS  ((volatile unsigned int*)(MAILBOX_BASE + 0x18)) // M0: Check GPU statue
#define MAILBOX_WRITE   ((volatile unsigned int*)(MAILBOX_BASE + 0x20)) // M1: CPU write to GPU

#define MAILBOX_EMPTY   0x40000000
#define MAILBOX_FULL    0x80000000

#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY      0x00010005
#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

// lab7
#define MBOX_REQUEST 0
#define MBOX_CH_PROP 8
#define MBOX_TAG_LAST 0

int mailbox_call(unsigned char ch, unsigned int *mailbox);
unsigned int get_board_revision();
void get_memory_info(unsigned int *base, unsigned int *size);

#endif
