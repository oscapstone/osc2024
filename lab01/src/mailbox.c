#include "mailbox.h"
#include "io.h"

void get_board_revision()
{
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

    // printf("0x%x\n", mailbox[5]); // it should be 0xa020d3 for rpi3 b+
    printf("\nBoard revision: ");
    printf_hex(mailbox[5]);
}

void get_memory_info()
{
    unsigned int mailbox[8];
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_ARM_MEMORY; // tag identifier
    mailbox[3] = 8; // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    mailbox[6] = 0;
    // tags end
    mailbox[7] = END_TAG;

    mailbox_call(mailbox); // message passing procedure call, you should implement it following the 6 steps provided above.

    printf("\nARM base memory address: ");
    printf_hex(mailbox[5]);
    printf("\nARM memory size: ");
    printf_hex(mailbox[6]);

}

void mailbox_call(unsigned int* mailbox)
{
    unsigned int mesg = (((unsigned long)mailbox) & ~0xf) | 8;

    while(*MAILBOX_STATUS & MAILBOX_FULL){   // // Check if Mailbox 0 status register’s full flag is set. if MAILBOX_STATUS == 0x80000001, then error parsing request buffer 
        asm volatile("nop");
    }

    *MAILBOX_WRITE = mesg;

    while(1){
        while(*MAILBOX_STATUS & MAILBOX_EMPTY){
            asm volatile("nop");
        }
        if(mesg == *MAILBOX_READ){
            break;
        }
    }
}