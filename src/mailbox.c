#include "mailbox.h"
#include "uart.h"

unsigned int msg[10];

int mailbox_exec(unsigned int channel)
{
    unsigned int r = ((unsigned int)((unsigned long)&msg & ~(0xF)) | (channel & 0xF));

    while (*MAILBOX_STATUS & MAILBOX_FULL) {
        asm volatile("nop");
    }

    *MAILBOX_WRITE = r;

    while (*MAILBOX_STATUS & MAILBOX_EMPTY) {
        asm volatile("nop");
    }
    while (r != *MAILBOX_READ) {
        asm volatile("nop");
    }
    return msg[1] == MAILBOX_RESPONSE;
}

void get_board_revision()
{
    msg[0] = 7*4;
    msg[1] = MAILBOX_REQUEST;
    msg[2] = MAILBOX_GET_BOARD_REVISION;
    msg[3] = 4;
    msg[4] = MAILBOX_BUF_REQUEST;
    msg[5] = 0;
    msg[6] = MAILBOX_END_TAG;

    if (mailbox_exec(MAILBOX_CH_ARM2VC)) {
        uart_puts("board revision:\n");
        uart_hex(msg[5]);
        uart_puts("\n");
    } else {
        uart_puts("Failed\n");
    }
}

void get_arm_memory()
{
    msg[0] = 8*4;
    msg[1] = MAILBOX_REQUEST;
    msg[2] = MAILBOX_GET_ARM_MEMORY;
    msg[3] = 8;
    msg[4] = MAILBOX_BUF_REQUEST;
    msg[5] = 0;
    msg[6] = 0;
    msg[7] = MAILBOX_END_TAG;

    if (mailbox_exec(MAILBOX_CH_ARM2VC)) {
        uart_puts("arm memory:\n");
        uart_puts("size in bytes:");
        uart_hex(msg[6]);
        uart_puts("\n");
        uart_puts("base address:");
        uart_hex(msg[5]);
        uart_puts("\n");
    } else {
        uart_puts("Failed\n");
    }
}