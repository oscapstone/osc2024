// Only MB 0 can trigger interrupts on the ARM.
// So MB0 is from VC to ARM and MB1 is from ARM to VC.

#ifndef __MAIXBOX_H
#define _MAILBOX_H

#define MMIO_BASE           0x3f000000+0x0000b880
#define MAILBOX_BASE        (volatile unsigned int*)(MMIO_BASE+0xb880)

#define MAILBOX_READ        (volatile unsigned int*)(MMIO_BASE)
#define MAILBOX_STATUS      (volatile unsigned int*)(MMIO_BASE+0x18)
#define MAILBOX_WRITE       (volatile unsigned int*)(MMIO_BASE+0x20)

#define MAILBOX_EMPTY       (volatile unsigned int)(0x40000000)
#define MAILBOX_FULL        (volatile unsigned int)(0x80000000)


#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY      0x00010005

#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0X00000000


int mailbox_call(unsigned int*);
void get_board_revision(void);
void get_arm_memory(void);

#endif