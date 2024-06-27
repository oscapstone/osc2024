#include "../include/mailbox.h"
#include "../include/mini_uart.h"



/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox_buf[8];

int mbox_call(unsigned char ch, unsigned int *mbox)
{
    /* 1. Combine the message address (upper 28 bits) with channel number (lower 4 bits) */
    unsigned int readchannel = (((unsigned int)((unsigned long)mbox) & ~0xF) | (0x8 & 0xF));

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
    mbox_buf[0] = 7 * 4; // buffer size in bytes
    mbox_buf[1] = REQUEST_CODE;
    // tags begin
    mbox_buf[2] = GET_BOARD_REVISION; // tag identifier
    mbox_buf[3] = 4;                  // maximum of request and response value buffer's length.
    mbox_buf[4] = TAG_REQUEST_CODE;
    mbox_buf[5] = 0; // value buffer
    // tags end
    mbox_buf[6] = END_TAG;
    mbox_call(0x8, mbox_buf); // message passing procedure call, you should implement it following the 6 steps provided above.
    uart_send_string("Board revision: ");
    uart_send_string("0x");
    uart_hex(mbox_buf[5]);
    uart_send_string("\r\n");
}

void get_base_address()
{
    mbox_buf[0] = 8 * 4; // buffer size in bytes
    mbox_buf[1] = REQUEST_CODE;
    // tags begin
    mbox_buf[2] = ARM_MEMORY; // tag identifier
    mbox_buf[3] = 8;          // maximum of request and response value buffer's length.
    mbox_buf[4] = TAG_REQUEST_CODE;
    mbox_buf[5] = 0; // value buffer
    mbox_buf[6] = 0; // value buffer
    // tags end
    mbox_buf[7] = END_TAG;
    mbox_call(0x8, mbox_buf); // message passing procedure call, you should implement it following the 6 steps provided above.
    uart_send_string("Arm base address: ");
    uart_send_string("0x");
    uart_hex(mbox_buf[5]);
    uart_send_string("\r\n");
    uart_send_string("Arm memory size: ");
    uart_send_string("0x");
    uart_hex(mbox_buf[6]);
    uart_send_string("\r\n");
}