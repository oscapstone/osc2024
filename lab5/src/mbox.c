#include "mbox.h"
#include "mini_uart.h"


int mailbox_call(unsigned char ch, unsigned int* mailbox){
    unsigned int r = (((unsigned long) mailbox) & ~0xF) | (ch & 0xf);
    // ~0xF is used to clear the last 4 bits of the address
    // 8 is used to set the last 4 bits to 8 (channel 8 is the mailbox channel)

    while(*MAILBOX_STATUS & MAILBOX_FULL){
        delay(1);
    } // wait until the mailbox is not full

    *MAILBOX_WRITE = r;

    while(1){
        while(*MAILBOX_STATUS & MAILBOX_EMPTY){
            delay(1);
        } // wait until the mailbox is not empty

        if(r == *MAILBOX_READ){
            return mailbox[1] == REQUEST_SUCCEED;
        }
    }
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

    mailbox_call((unsigned char)0x8, mailbox); // message passing procedure call, you should implement it following the 6 steps provided above.

    uart_send_string("Board revision: ");
    uart_hex(mailbox[5]);
    uart_send_string("\n");
}

void get_memory_info() {
    unsigned int mailbox[8];
    mailbox[0] = 8 * 4;  // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_ARM_MEMORY;    // tag identifier
    mailbox[3] = 8;                 // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;  // tag code
    mailbox[5] = 0;                 // base address
    mailbox[6] = 0;                 // size in bytes
    mailbox[7] = END_TAG;           // end tag
    // tags end
    mailbox_call((unsigned char)0x8, mailbox);
    uart_send_string("ARM memory base address : ");
    uart_hex(mailbox[5]);
    uart_send_string("\n");

    uart_send_string("ARM memory size : ");
    uart_hex(mailbox[6]);
    uart_send_string("\n");
    
}