#include "gpio.h"
#include "mailbox.h"
#include "uart.h"

volatile unsigned int __attribute__((aligned(16))) mailbox[8];

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

int mailbox_call(){
    unsigned int r = (((unsigned int)((unsigned long)&mailbox) & ~0xF) | (0x8 & 0xF));

    // do{ asm volatile("nop");} while(*MBOX_STATUS & MBOX_FULL);
    while(*MBOX_STATUS & MBOX_FULL){}

    *MBOX_WRITE = r;

    while (1) {
        while(*MBOX_STATUS & MBOX_FULL){}
        // do{ asm volatile("nop");} while(*MBOX_STATUS & MBOX_EMPTY);
        if (r == *MBOX_READ){
            return mailbox[1] == MBOX_RESPONSE;
        }
    }
    return 0;
}

void get_board_revision(){
    mailbox[0] = 7 * 4; // buffer size in bytes
    mailbox[1] = MAILBOX_REQ; // request
    // tags begin
    mailbox[2] = MAILBOX_TAG_GETBOARD; // tag identifier
    mailbox[3] = 4;                  // maximum of request and response value buffer's length.
    mailbox[4] = MAILBOX_TAG_CODE;
    mailbox[5] = 0; // value buffer
    // tags end
    mailbox[6] = MAILBOX_TAG_LAST;
    
    unsigned int a = mailbox_call(); // message passing procedure call, you should implement it following the 6 steps provided above.
}
void get_arm_memory(){
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = MAILBOX_REQ;
    // tags begin
    mailbox[2] = MAILBOX_TAG_GETARMMEM; // tag identifier
    mailbox[3] = 8;          // maximum of request and response value buffer's length.
    mailbox[4] = MAILBOX_TAG_CODE;
    mailbox[5] = 0; // value buffer
    mailbox[6] = 0; // value buffer
    // tags end
    mailbox[7] = MAILBOX_TAG_LAST;

    unsigned int a = mailbox_call(); // message passing procedure call, you should implement it following the 6 steps provided above.
}

