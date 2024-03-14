#ifndef	_P_MBOX_H
#define	_P_MBOX_H

#include "peripherals/base.h"


#define MAILBOX_BASE    PBASE + 0xb880

// 因為 r = *MAILBOX_READ; 會出錯 所以轉成 unsigned int* 指標
#define MAILBOX_READ    (unsigned int*)(MAILBOX_BASE + 0x00) 
#define MAILBOX_STATUS  (unsigned int*)(MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE   (unsigned int*)(MAILBOX_BASE + 0x20)

#define MAILBOX_EMPTY   0x40000000
#define MAILBOX_FULL    0x80000000

// extern volatile unsigned int mbox[36];


// /* tags */
#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY      0x00010005

#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001

#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

#endif  /*_P_MBOX_H */


