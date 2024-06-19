#include "mbox.h"
#include "uart.h"
/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[72];

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch) // Mailbox 0 define several channels, but we only use channel 8 (CPU->GPU) for communication.
{
    unsigned long r = (((unsigned long)((unsigned long)mbox) & ~0xF) | (ch & 0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if(r == PHYS_TO_VIRT(*MBOX_READ))
            /* is it a valid successful response? */
            return mbox[1]==MBOX_RESPONSE;  
    }
    return 0;
}

