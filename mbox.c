#include "gpio.h"
#include "uart.h"
#include "mbox.h"

/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[8];

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch)
{
    unsigned int r = (((unsigned int)((unsigned long)&mbox)&~0xF) | (ch&0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if(r == *MBOX_READ)
            /* is it a valid successful response? */
            return mbox[1]==MBOX_RESPONSE;
    }
    return 0;
}

void show_info(){
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message    
    mbox[2] = MBOX_TAG_GETREVISION; // get serial number command
    mbox[3] = 8;                    // buffer size
    mbox[4] = TAG_REQUEST_CODE;     
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST;
    
    if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("Board Revision : ");
        uart_hex(mbox[5]);
        uart_puts("\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message  
    mbox[2] = MBOX_TAG_GETARMMEM;
    mbox[3] = 8;                    // buffer size
    mbox[4] = TAG_REQUEST_CODE;  
    mbox[5] = 0;                 
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST;
    if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("ARM memory base address in bytes : ");
        uart_hex(mbox[5]);
        uart_puts("\n");
        uart_puts("ARM memory size in bytes : ");
        uart_hex(mbox[6]);
        uart_puts("\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }
}