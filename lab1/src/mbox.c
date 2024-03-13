#include "mini_uart.h"
#include "peripherals/mbox.h"


int mbox_call(unsigned int* mbox, unsigned char channel) {

    unsigned int r = (unsigned int)(((unsigned long)mbox) & (~0xF)) | (channel & 0xF);

    // wait until full flag unset
    while (*MAILBOX_STATUS & MAILBOX_FULL) {
    }

    // write address of message + channel to mailbox
    *MAILBOX_WRITE = r;

    // wait until response
    while (1) {
        // wait until empty flag unset
        while (*MAILBOX_STATUS & MAILBOX_EMPTY) {
        }

        // is it a response to our msg?
        if (r == *MAILBOX_READ) {
            // check is response success
            return mbox[1] == REQUEST_SUCCEED;
        }
    }
    return 0;
}

void get_board_revision()
{
    unsigned int mbox[7];   // mailbox message buffer
    mbox[0] = 7 * 4;        // buffer size in bytes
    mbox[1] = REQUEST_CODE; // request: 0x00000000 , response: 0x80000000(success)
    // tag begin
    mbox[2] = GET_BOARD_REVISION; // tag identifier
    mbox[3] = 4;                  // value buffer size in bytes
    mbox[4] = TAG_REQUEST_CODE;   // bit31 clear: request , bit31 set: response
    mbox[5] = 0;                  // value buffer
    // tag end
    mbox[6] = END_TAG;
    mbox_call(mbox, 8); // use channel 8 (CPU -> GPU)
    uart_send_string("Board's revision is ");
    uart_hex(mbox[5]);
    uart_send_string("\r\n");
    return;
}

void get_arm_memory()
{
    unsigned int mbox[8]; // mailbox message buffer
    mbox[0] = 8 * 4;      // buffer size in bytes
    mbox[1] = REQUEST_CODE;
    // tag begin
    mbox[2] = GET_ARM_MEMORY; // tag identifier
    mbox[3] = 8;              // length : 8
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0; // base addr
    mbox[6] = 0; // size in bytes
    // tag end
    mbox[7] = END_TAG;
    mbox_call(mbox, 8); // use channel 8 (CPU -> GPU)
    uart_send_string("ARM's memory base is ");
    uart_hex(mbox[5]);
    uart_send_string("	, and size is ");
    uart_hex(mbox[6]);
    uart_send_string("\r\n");
    return;
}