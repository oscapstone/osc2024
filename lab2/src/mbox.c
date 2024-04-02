#include "mbox.h"
#include "mini_uart.h"

int mbox_call(unsigned int *mbox, unsigned char ch)
{
    unsigned int r = (((unsigned int)((unsigned long)mbox) & ~0xF) |
                      (ch & 0xF)); // combine mbox(upper 28bit) with channel(lower 4bit)
    while (*MAILBOX_STATUS & MAILBOX_FULL)
    {
    }                   // check if MAILBOX0 status reg's full flag
    *MAILBOX_WRITE = r; // write to MAILBOX1 read/write reg
    while (1)
    {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY)
        {
        }                       // check if MAILBOX0 status reg's empty flag
        if (r == *MAILBOX_READ) // check if it is the response to our message
        {
            return mbox[1] == REQUEST_SUCCEED; // check if response's mbox[1] is success
        }
    }
    return 0; // 0 : fail
}

void get_board_revision()
{
    unsigned int __attribute__((aligned(16))) mbox[7];   // mailbox message buffer
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
    uart_send_string("\n\r");
    return;
}

void get_mem_info()
{
    unsigned int __attribute__((aligned(16))) mbox[8]; // mailbox message buffer
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
    uart_send_string("\n\n\r");
    return;
}
