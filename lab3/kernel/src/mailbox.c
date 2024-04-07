#include "mailbox.h"
#include "uart.h"

#define MMIO_BASE 0x3f000000
#define MAILBOX_BASE (MMIO_BASE + 0x0000B880)
#define MAILBOX_READ ((volatile unsigned int *)(MAILBOX_BASE + 0x0))
#define MAILBOX_POLL ((volatile unsigned int *)(MAILBOX_BASE + 0x10))
#define MAILBOX_SENDER ((volatile unsigned int *)(MAILBOX_BASE + 0x14))
#define MAILBOX_STATUS ((volatile unsigned int *)(MAILBOX_BASE + 0x18))
#define MAILBOX_CONFIG ((volatile unsigned int *)(MAILBOX_BASE + 0x1C))
#define MAILBOX_WRITE ((volatile unsigned int *)(MAILBOX_BASE + 0x20))
#define MAILBOX_RESPONSE 0x80000000
#define MAILBOX_FULL 0x80000000
#define MAILBOX_EMPTY 0x40000000

volatile unsigned int __attribute__((aligned(16))) mailbox[36];

int mailbox_call(unsigned char ch)
{
    unsigned int r = ((unsigned int)((unsigned long)&mailbox) | (ch & 0xF)); //Combine the message

    while (*MAILBOX_STATUS & MAILBOX_FULL); // Wait until Mailbox 0 status register's full flag is unset.

    *MAILBOX_WRITE = r; // Write to Mailbox 0 Read/Write register.

    while (1)
    {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY); // Wait until Mailbox 0 status register's empty flag is unset.
        if (r == *MAILBOX_READ) // Read from Mailbox 0 Read/Write register and check if the value is same as before.
            return mailbox[1] == MAILBOX_RESPONSE;
    }

    return 0;
}

#define GET_BOARD_REVISION 0x00010002
#define GET_ARM_MEMORY 0x00010005
#define REQUEST_CODE 0x00000000
#define REQUEST_SUCCEED 0x80000000
#define REQUEST_FAILED 0x80000001
#define TAG_REQUEST_CODE 0x00000000
#define END_TAG 0x00000000

void get_board_revision()
{
    mailbox[0] = 7 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION; // tag identifier
    mailbox[3] = 4;                  // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    // tags end
    mailbox[6] = END_TAG;

    if (mailbox_call(8))
    {
        uart_puts("board version: 0x");
        uart_hex_lower_case(mailbox[5]); // it should be 0xa020d3 or 0xa020d4 for rpi3 b+
        uart_puts("\n");
    }
    else
        uart_puts("fail\n");
}

void get_arm_memory()
{
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_ARM_MEMORY; // tag identifier
    mailbox[3] = 8;              // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    mailbox[6] = 0;
    // tags end
    mailbox[7] = END_TAG;

    if (mailbox_call(8))
    {
        uart_puts("base address: 0x");
        uart_hex_upper_case(mailbox[5]);
        uart_puts("\n");
        uart_puts("size: 0x");
        uart_hex_upper_case(mailbox[6]);
        uart_puts("\n");
    }
    else
        uart_puts("fail\n");
}