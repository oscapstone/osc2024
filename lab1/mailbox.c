#include "headers/mailbox.h"
#include "headers/mini_uart.h"

#define MMIO_BASE       (0x3F000000)
#define MAILBOX_BASE    (MMIO_BASE + 0xB880)

#define MAILBOX_READ    (MAILBOX_BASE)
#define MAILBOX_STATUS  (MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE   (MAILBOX_BASE + 0x20)

#define MAILBOX_EMPTY   (0x40000000)
#define MAILBOX_FULL    (0x80000000)

#define MAILBOX_CHANNEL (0x8)
static volatile unsigned int __attribute__((aligned(16))) mailbox[ MAILBOX_WRITE - MAILBOX_BASE];

static int mailbox_call()
{
    unsigned int reg = (unsigned int)((((unsigned long)mailbox) & ~0xF) | ( MAILBOX_CHANNEL & 0xF));

    // check channel status
    while ( *(volatile unsigned int *)MAILBOX_STATUS & MAILBOX_FULL)
    {
        asm volatile("nop");
    }// while
    
    *(volatile unsigned int *)MAILBOX_WRITE = reg;

    // check if successful
    while (1)
    {
        while ( *(volatile unsigned int *)MAILBOX_STATUS & MAILBOX_EMPTY)
        {
            asm volatile("nop");
        }// while
        if ( reg == *(volatile unsigned int *)MAILBOX_READ)
        {
            // check for finish
            return mailbox[ 1] == MAILBOX_FULL;
        }// if
    }// while

    return 0;
}

// the request codes
#define GET_ARM_MEMORY      0x00010005
#define GET_BOARD_REVISION  0x00010002
#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

void mailbox_get_board_revision()
{
    mailbox[0] = 7 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION; // tag identifier
    mailbox[3] = 4; // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    // tags end
    mailbox[6] = END_TAG;

    mini_uart_puts("\r\n");
    if ( mailbox_call())
    {
        mini_uart_puts("Board revision: 0x");
        mini_uart_puthexint( mailbox[ 5]);
    }// if
    else
    {
        mini_uart_puts("Board revision call: failed");
    }// else
    
    return;
}

void mailbox_get_arm_base_size()
{
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

    mini_uart_puts("\r\n");
    if ( mailbox_call())
    {
        mini_uart_puts("Base address: 0x");
        mini_uart_puthexint( mailbox[ 5]);
        mini_uart_puts("\r\n");
        mini_uart_puts("size: 0x");
        mini_uart_puthexint( mailbox[ 6]);
    }// if
    else
    {
        mini_uart_puts("Arm address call: failed");
    }// else
    
    return;
}
