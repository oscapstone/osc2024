#include "mbox.h"
#include "uart.h"

void mailbox_call(unsigned int *mbox)
{
    unsigned int r = (((unsigned int)(unsigned long)mbox) & ~0xF) | 0x8;

    while (*MAILBOX_STATUS & MAILBOX_FULL) {
        asm volatile("nop");
    }

    *MAILBOX_WRITE = r;

    while (1) {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY) {
            asm volatile("nop");
        }
        if (r == *MAILBOX_READ) {
            return;
        }
    }
}

void get_board_revision()
{
    unsigned int mailbox[7];
    mailbox[0] = 7 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION; // tag identifier
    mailbox[3] = 4;                  // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    // tags end
    mailbox[6] = END_TAG;

    mailbox_call(
        mailbox); // message passing procedure call, you should implement it following the 6 steps provided above.

    uart_puts("Board revision: ");
    uart_hex(mailbox[5]); // it should be 0xa020d3 for rpi3 b+
}

void get_memory_info()
{
    unsigned int mailbox[8];
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_ARM_MEMORY; // tag identifier
    mailbox[3] = 8;              // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    mailbox[6] = 0; // value buffer
    // tags end
    mailbox[7] = END_TAG;

    mailbox_call(
        mailbox); // message passing procedure call, you should implement it following the 6 steps provided above.

    uart_puts("ARM memory base: ");
    uart_hex(mailbox[5]); // it should be 0x3b for rpi3 b+

    uart_puts("ARM memory size: ");
    uart_hex(mailbox[6]); // it should be 0x40000000 for rpi3 b+
}