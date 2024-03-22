#include "type.h"
#include "mmio.h"
#include "mailbox.h"


/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[36];

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)

#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x00))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x20))


/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
uint32_t 
mailbox_call(unsigned char ch)
{
    /* step 1. Combine the message address (upper 28 bits) with channel number (lower 4 bits) */
    unsigned int r = (((unsigned int)((unsigned long) &mbox) & ~0xF) | (ch & 0xF));

    /* step 2. Check if Mailbox 0 status register's full flag is set */ 
    /* wait until we can write to the mailbox */
    do { asm volatile("nop"); } while (*MBOX_STATUS & MBOX_FULL);
    
    /* step 3. If not full, then you can write to Mailbox 1 Read/Write register */
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    
    /* now wait for the response */
    while(1) {
        
        /* 4. Check if Mailbox 0 status register's empty flag is set. */
        /* is there a response? */
        do { asm volatile("nop"); } while (*MBOX_STATUS & MBOX_EMPTY);
        
        /* 5. If not empty, then you can read from Mailbox 0 Read/Write register. */
        /* is it a response to our message? */
        if (r == *MBOX_READ) {
            /* 6. Check if the value is the same as you wrote in step 1. */
            /* is it a valid successful response? */
            return mbox[1] == MBOX_RESPONSE;
        }
    }
    return 0;
}
