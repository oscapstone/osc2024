#include "../include/peripherals/mbox.h"

#include "uart.h"

int mbox_call(unsigned int* mbox, unsigned char channel) {
    // create 28 bit data + 4 bit channel
    unsigned int r = (unsigned int)(((unsigned long)mbox) & (~0xF)) | (channel & 0xF);
    // wait until full flag unset
    // MBOX_STATUS : 0x18
    // MBX_FULL : Status register bits  
    // bit 31 :	Mailbox full 
    while (*MBOX_STATUS & MBOX_FULL) {
    }
    // write address of message + channel to mailbox
    // write to mbox1
    *MBOX_WRITE = r;
    // wait until response
    while (1) {
        // wait until empty flag unset
        while (*MBOX_STATUS & MBOX_EMPTY) {
        }
        // is it a response to our msg?
        if (r == *MBOX_READ) {
            // check is response success
            return mbox[1] == MBOX_CODE_BUF_RES_SUCC;
        }
    }
    return 0;
}

void mbox_board_revision() {
    unsigned int __attribute__((aligned(16))) mbox[7];
    mbox[0] = 7 * 4;  // buffer size in bytes
    mbox[1] = MBOX_CODE_BUF_REQ;
    // tags begin
    mbox[2] = MBOX_TAG_GET_BOARD_REVISION;  // tag identifier
    mbox[3] = 4;                            // Tag return value buffer size in bytes 
    mbox[4] = 0;                            // tag status, 	Garbage with bit 31 cleared
    mbox[5] = 0;                            // value buffer
    mbox[6] = 0x0;                          // end tag
    // tags end
    mbox_call(mbox, 8);
    uart_printf("Board Revision: %x\n", mbox[5]);
}

void mbox_arm_memory() {
    unsigned int __attribute__((aligned(16))) mbox[8];
    mbox[0] = 8 * 4;  // buffer size in bytes
    mbox[1] = MBOX_CODE_BUF_REQ;
    // tags begin
    mbox[2] = MBOX_TAG_GET_ARM_MEMORY;  // tag identifier
    mbox[3] = 8;                       // Tag return value buffer size in bytes 
    mbox[4] = 0;                       // tag status, 	Garbage with bit 31 cleared
    mbox[5] = 0;                       // value buffer
    mbox[6] = 0;                       // value buffer
    mbox[7] = 0x0;                     // end tag
    // tags end
    mbox_call(mbox, 8);
    uart_printf("Arm base addr: 0x%x size: 0x%x\n", mbox[5], mbox[6]);
}