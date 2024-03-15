#include "mbox.h"

volatile unsigned int __attribute__((aligned(16))) mbox[36];

int mailbox_call(unsigned char c)
{
    unsigned int r =
        ((unsigned int)(((unsigned long)&mbox & ~0xF) | (c & 0xF)));

    // Wait until the mailbox is not full
    while (*MAILBOX_STATUS & MAILBOX_FULL)
        asm volatile("nop");

    // Write to the register
    *MAILBOX_WRITE = r;

    while (1) {
        // Wait for the response
        while (*MAILBOX_STATUS & MAILBOX_EMPTY)
            asm volatile("nop");

        if (r == *MAILBOX_READ)
            return mbox[1] == MAILBOX_RESPONSE;
    }
    return 0;
}