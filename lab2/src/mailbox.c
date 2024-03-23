#include "../include/mailbox.h"
#include "../include/mini_uart.h"



/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[8];

int mbox_call()
{
    /* 1. Combine the message address (upper 28 bits) with channel number (lower 4 bits) */
    unsigned int readchannel = (((unsigned int)((unsigned long)&mbox) & ~0xF) | (0x8 & 0xF));

    /* 2. Check if Mailbox 0 status register’s full flag is set.*/
    while (*MAILBOX_STATUS & MAILBOX_FULL);

    /* 3. If not, then you can write to Mailbox 1 Read/Write register.*/
    *MAILBOX_WRITE = readchannel;

    /* 4. Check if Mailbox 0 status register’s empty flag is set.*/
    while (1) {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY);
        if (readchannel == *MAILBOX_READ) {
            return mbox[1] == MAILBOX_RESPONSE;
        }
    }
    return 0;
}

void get_board_revision()
{
    uart_send_string("In get_board_revision\r\n");
    mbox[0] = 7 * 4; // buffer size in bytes
    mbox[1] = REQUEST_CODE;
    // tags begin
    mbox[2] = GET_BOARD_REVISION; // tag identifier
    mbox[3] = 4;                  // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0; // value buffer
    // tags end
    mbox[6] = END_TAG;
    mbox_call(); // message passing procedure call, you should implement it following the 6 steps provided above.
    uart_send_string("Board revision: ");
    uart_send_string("0x");
    uart_hex(mbox[5]);
    uart_send_string("\r\n");
}

void get_base_address()
{
    mbox[0] = 8 * 4; // buffer size in bytes
    mbox[1] = REQUEST_CODE;
    // tags begin
    mbox[2] = ARM_MEMORY; // tag identifier
    mbox[3] = 8;          // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0; // value buffer
    mbox[6] = 0; // value buffer
    // tags end
    mbox[7] = END_TAG;
    mbox_call(); // message passing procedure call, you should implement it following the 6 steps provided above.
    uart_send_string("Arm base address: ");
    uart_send_string("0x");
    uart_hex(mbox[5]);
    uart_send_string("\r\n");
    uart_send_string("Arm memory size: ");
    uart_send_string("0x");
    uart_hex(mbox[6]);
    uart_send_string("\r\n");
}