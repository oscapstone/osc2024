#include "peripherals/mailbox.h"
#include "mailbox.h"
#include "mini_uart.h"

void printhex (unsigned int n) {
    char out[9];
    out[8] = '\0';
    for (int i = 7; i >= 0; --i) {
        unsigned int o = n % 16;
        n >>= 4;
        switch (o) {
        case 10:
            out[i] = 'A';
            break;
        case 11:
            out[i] = 'B';
            break;
        case 12:
            out[i] = 'C';
            break;
        case 13:
            out[i] = 'D';
            break;
        case 14:
            out[i] = 'E';
            break;
        case 15:
            out[i] = 'F';
            break;
        default:
            out[i] = '0' + o;
            break;
        }
    }
    uart_send_string("0x");
    uart_send_string(out);
}

int mailbox_call (volatile unsigned int *mailbox) {
    unsigned int msg = ((unsigned long)mailbox & ~0xF) | (0x8 & 0xF);
    // only use channel 8 (CPU->GPU) for communication (lower 4 bits)
    do{asm volatile("nop");}while(*MAILBOX_STATUS & MAILBOX_FULL);
    *MAILBOX_WRITE = msg;
    while (1) {
        do{asm volatile("nop");}while(*MAILBOX_STATUS & MAILBOX_EMPTY);
        if (msg == *MAILBOX_READ) {
            return mailbox[1];
        }
    }
}

void get_board_revision ()
{
    volatile unsigned int __attribute__((aligned(16))) mailbox[7];
    mailbox[0] = 7 * 4;  // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION;  // tag identifier
    mailbox[3] = 4;  // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;  // value buffer
    // tags end
    mailbox[6] = END_TAG;

    if (mailbox_call(mailbox) == REQUEST_SUCCEED) {
        uart_send_string("Board Revision:\t\t");
        printhex(mailbox[5]);
        uart_send_string("\r\n");
    }
}

void get_arm_memory ()
{
    volatile unsigned int __attribute__((aligned(16))) mailbox[8];
    mailbox[0] = 8 * 4;
    mailbox[1] = REQUEST_CODE;
    mailbox[2] = GET_ARM_MEMORY;
    mailbox[3] = 8;
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;
    mailbox[6] = 0;
    mailbox[7] = END_TAG;

    if (mailbox_call(mailbox) == REQUEST_SUCCEED) {
        uart_send_string("Memory Base Address:\t");
        printhex(mailbox[5]);
        uart_send_string("\r\n");
        uart_send_string("Memory Size:\t\t");
        printhex(mailbox[6]);
        uart_send_string("\r\n");
    }
}
