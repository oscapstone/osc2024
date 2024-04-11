#include "peripherals/mailbox.h"
#include "mini_uart.h"

void mailbox_call(unsigned int *mailbox, unsigned char channel)
{
    unsigned int r = (((unsigned long)mailbox) & ~0xf) | (channel & 0xF);
    
    // wait until we can write to the mailbox 
    while(*MAILBOX_STATUS & MAILBOX_FULL)       // wait until full flag unset
    {
        continue;
    }
    *MAILBOX_WRITE = r;                         // write address of message + channel to mailbox

    while(1)                                    // wait until response
    {
        while(*MAILBOX_STATUS & MAILBOX_EMPTY)  // wait until empty flag unset
        {
            continue;
        }
        if(r == *MAILBOX_READ)
        {
            return;
        }
    }
}

void hardware_board_revision()
{
    unsigned int __attribute__((aligned(16))) mailbox[7];   // make sure align with 16 bits
    mailbox[0] = 7*4;                                       // first 16 bits -> indicate length of mailbox, unit is 4bits
    mailbox[1] = REQUEST_CODE;                              // second -> command of mailbox
    // tags begin    
    mailbox[2] = GET_BOARD_REVISION;                        // mailbox tag
    mailbox[3] = 4;                                         // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;                                         // value buffer
    // tags end
    mailbox[6] = END_TAG;

    mailbox_call(mailbox, 8);
    uart_send_string("Board Revision: 0x");
    uart_send_string_int2hex(mailbox[5]);
    uart_send_string("\r\n");
}

void hardware_vc_memory()
{
    unsigned int __attribute__((aligned(16))) mailbox[8];
    mailbox[0] = 8*4;
    mailbox[1] = REQUEST_CODE;
    // tags begin    
    mailbox[2] = TAG_GET_ARM_MEMORY;
    mailbox[3] = 8;                                         // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;                                         // base address
    mailbox[6] = 0;                                         // size in bytes
    // tags end
    mailbox[7] = END_TAG;

    mailbox_call(mailbox, 8);
    uart_send_string("VC Core base address: 0x");
    uart_send_string_int2hex(mailbox[5]);
    uart_send_string(" memory size: 0x");
    uart_send_string_int2hex(mailbox[6]);
    uart_send_string("\r\n");
    
}
