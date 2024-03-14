#include "mailbox.h"

/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mailbox[36];

int mailbox_call(){
    unsigned int r = (((unsigned int)((unsigned long)&mailbox)&~0xF) | (mailbox_prop_arm_vc&0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MAILBOX_STATUS & MAILBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MAILBOX_WRITE = r;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MAILBOX_STATUS & MAILBOX_EMPTY);
        /* is it a response to our message? */
        if(r == *MAILBOX_READ)
            /* is it a valid successful response? */
            return mailbox[1]==MAILBOX_RESPONSE;
    }
    return 0;
}



void get_board_revision(){
    mailbox[0] = 7 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION; // tag identifier
    mailbox[3] = 4; // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    // tags end
    mailbox[6] = END_TAG;

    mailbox_call(); // message passing procedure call, you should implement it following the 6 steps provided above.
}



void get_arm_mem(){
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_ARM_MEM; // tag identifier
    mailbox[3] = 8; // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    mailbox[6] = 0; // u32: base address in bytes, u32: size in bytes
    // tags end
    mailbox[7] = END_TAG;

    mailbox_call(); // message passing procedure call, you should implement it following the 6 steps provided above.
}
