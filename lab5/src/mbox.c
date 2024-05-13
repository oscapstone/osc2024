#include "mbox.h"
#include "uart.h"

int mbox_call(unsigned char ch, unsigned int *mbox)
{
    unsigned int r = ((unsigned long)mbox & ~0xF) | (ch & 0xF);
    // Wait until we can write to the mailbox
    while (*MAILBOX_STATUS & MAILBOX_FULL)
        ;
    *MAILBOX_WRITE = r; // Write the request
    while (1) {
        // Wait for the response
        while (*MAILBOX_STATUS & MAILBOX_EMPTY)
            ;
        if (r == *MAILBOX_READ)
            return mbox[1] == MAILBOX_RESPONSE;
    }
    return 0;
}

void get_revision()
{
#define GET_BOARD_REVISION 0x00010002
    unsigned int __attribute__((aligned(16))) mbox[7];
    mbox[0] = 7 * 4;
    mbox[1] = REQUEST_CODE;
    mbox[2] = GET_BOARD_REVISION;
    mbox[3] = 4;
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;
    mbox[6] = END_TAG;
    if (mbox_call(MAILBOX_CH_PROP, mbox)) {
        uart_puts("board revision: ");
        uart_hex(mbox[5]);
        uart_putc('\n');
    }
}

void get_mem_info()
{
#define GET_ARM_MEM_INFO 0x00010005
    unsigned int __attribute__((aligned(16))) mbox[8];
    mbox[0] = 8 * 4;
    mbox[1] = REQUEST_CODE;
    mbox[2] = GET_ARM_MEM_INFO;
    mbox[3] = 8;
    mbox[4] = 0;
    mbox[5] = 0;
    mbox[6] = 0;
    mbox[7] = END_TAG;
    if (mbox_call(MAILBOX_CH_PROP, mbox)) {
        uart_puts("device base memory address: ");
        uart_hex(mbox[5]);
        uart_putc('\n');
        uart_puts("device memory size: ");
        uart_hex(mbox[6]);
        uart_putc('\n');
    }
}