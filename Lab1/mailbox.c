// from raspi3-tutorial 04_mailboxes/mbox.c
#include "mailbox.h"
#include "io.h"

void mailbox_call(unsigned int* message) {
    unsigned int  r = (((unsigned long)message) & ~0xF) | 8;

    while (*MAILBOX_STATUS & MAILBOX_FULL)
        asm volatile("nop");

    *MAILBOX_WRITE = r;

    while (1) {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY)
            asm volatile("nop");

        if (r == *MAILBOX_READ)
            break;
    }
    return;
}


void get_board_revision(){
    unsigned int mailbox[7];
    mailbox[0] = 7 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION; // tag identifier
    mailbox[3] = 4; // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    // tags end
    mailbox[6] = END_TAG;

    
    mailbox_call(mailbox); // message passing procedure call, you should implement it following the 6 steps provided above.

    print_string("board revision : ");
    print_h(mailbox[5]);
    print_string("\r\n");
}

void p_mem_info(){
    unsigned int mailbox[8];
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_ARM_MEMORY; // tag identifier
    mailbox[3] = 8; // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    mailbox[6] = 0; // value buffer
    // tags end
    mailbox[7] = END_TAG;

    mailbox_call(mailbox); // message passing procedure call, you should implement it following the 6 steps provided above.

    print_string("ARM base address : ");
    print_h(mailbox[5]);
    print_string("\r\n");
    print_string("ARM size : ");
    print_h(mailbox[6]);
    print_string("\r\n");
}