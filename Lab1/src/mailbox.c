#include "mailbox.h"
#include "uart.h"

/* a properly aligned buffer */
VUI  __attribute__((aligned(16))) mbox[36];

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch)
{
    UI r = (((UI)((UL)&mbox) & ~0xF) | (ch & 0xF));
    
    /* wait until we can write to the mailbox */
    do {asm volatile("nop");} while (*MBOX_STATUS & MBOX_FULL);
    
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while(1) 
    {
        /* is there a response? */
        do {asm volatile("nop");} while (*MBOX_STATUS & MBOX_EMPTY);
        
        /* is it a response to our message? */
        if(r == *MBOX_READ)
            /* is it a valid successful response? */
            return mbox[1]==MBOX_RESPONSE;
    }
    return 0;
}

void print_hd_info()
{
    uart_puts("Hardware information:\n\n");

    // print hw revision
    mbox[0] = 8 * 4;
    mbox[1] = MBOX_REQUEST_PROCESS;
    mbox[2] = MBOX_TAG_GET_BOARD_REVISION;
    mbox[3] = 4;
    mbox[4] = MBOX_TAG_REQUEST_CODE;
    mbox[5] = 0;
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST_BYTE;
    if (mbox_call(MBOX_TAGS_ARM_TO_VC))
    {
        uart_puts("Hardware Revision\t: ");
        uart_2hex(mbox[6]);
        uart_2hex(mbox[5]);
        uart_puts("\n");
    }
    else
    {
        uart_puts("Fail: Hardware Revision\n");
    }

    // print arm memory
    mbox[0] = 8 * 4;
    mbox[1] = MBOX_REQUEST_PROCESS;
    mbox[2] = MBOX_TAG_GET_ARM_MEMORY;
    mbox[3] = 8;
    mbox[4] = MBOX_TAG_REQUEST_CODE;
    mbox[5] = 0;
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST_BYTE;
    if (mbox_call(MBOX_TAGS_ARM_TO_VC)) 
    {
        uart_puts("ARM Memory Base Address\t: ");
        uart_2hex(mbox[5]);
        uart_puts("\n");
        uart_puts("ARM Memory Size\t\t: ");
        uart_2hex(mbox[6]);
        uart_puts("\n");
    }
    else
    {
        uart_puts("Fail: Memory info\n");
    }

    uart_puts("\n\n");
}
