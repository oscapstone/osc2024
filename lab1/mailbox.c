#include "headers/mailbox.h"
#include "headers/uart.h"
#include "headers/utils.h"

// Channel 8: Request from ARM; Channel 9: Request from VC.
// buffer contents:
// (u32) 1. buffer size(bytes)
// (u32) 2. Request/Response code
// (u??) 3. tags+end_tag+padding
//
// tags format:
// (u32) 1. tag id
// (u32) 2. value buffer size(bytes)
// (u32) 3. Request/Response
// (u32) 4. value buffer(need padding)

void get_board_revision()
{
    unsigned int mailbox[7];
    mailbox[0] = 4*7;
    mailbox[1] = REQUEST_CODE;
    mailbox[2] = GET_BOARD_REVISION;
    mailbox[3] = 4;
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;
    mailbox[6] = END_TAG;

    mailbox_call(mailbox);

    char hex[8];
    bin2hex(mailbox[5], hex);
    display("Board revision: ");
    display("0x"); display(hex); display("\n");
}

void get_arm_memory()
{
    unsigned int mailbox[8];
    mailbox[0] = 4*8;
    mailbox[1] = REQUEST_CODE;
    mailbox[2] = GET_ARM_MEMORY;
    mailbox[3] = 8;
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;
    mailbox[6] = 0;
    mailbox[7] = END_TAG;

    mailbox_call(mailbox);

    char hex[8];
    bin2hex(mailbox[5], hex);
    display("ARM Memory base address: ");
    display("0x"); display(hex); display("\n");
    
    bin2hex(mailbox[6], hex);
    display("ARM Memory size: ");
    display("0x"); display(hex); display("\n");

}

int mailbox_call(unsigned int *mailbox)
{
    unsigned int message = ((unsigned int)((unsigned long)(mailbox))&0xfffffff0) | (0x8&0xf);
    do{asm volatile("nop");}while((*MAILBOX_STATUS)==MAILBOX_FULL);
    *MAILBOX_WRITE = message;
    while(1)
    {
        do{asm volatile("nop");}while((*MAILBOX_STATUS)==MAILBOX_EMPTY);
        if(message == *MAILBOX_READ)
            return mailbox[1]==0x80000000;
    }
}