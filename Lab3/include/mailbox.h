#ifndef __MAILBOX_H__
#define __MAILBOX_H__

#include "base.h"


#define MAILBOX_READ    (volatile unsigned int*)(MAILBOX_BASE)           // CPU(host) read from GPU(device)
#define MAILBOX_STATUS  (volatile unsigned int*)(MAILBOX_BASE + 0x18)  // check GPU(device) status
#define MAILBOX_WRITE   (volatile unsigned int*)(MAILBOX_BASE + 0x20)  // CPU(host) write to GPU(device)

#define MAILBOX_EMPTY   0x40000000
#define MAILBOX_FULL    0x80000000

#define CHANNEL8 0x8 //8?

#define GET_BOARD_REVISION 0x00010002
#define GET_ARM_MEMORY     0x00010005
#define REQUEST_CODE       0x00000000
#define REQUEST_SUCCESS    0x80000000
#define REQUEST_FAILED     0x80000001
#define TAG_REQUEST_CODE   0x00000000
#define END_TAG             0x00000000

int mailbox_call(volatile unsigned int* message);
void get_board_revision();
void get_arm_memory();

#endif