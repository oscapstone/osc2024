#include "gpio.h"

/* mailbox message aligned buffer  */
// Because only upper 28 bits of message address could be passed, the message array should be correctly aligned.
volatile unsigned int  __attribute__((aligned(16))) mbox[36];

// mailbox register
#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880) // mailbox physical address
#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

/**
 * Send a message to a mailbox channel and wait for a response.
 * Returns 0 on failure, non-zero on success.
 */
int mbox_call(unsigned char ch) {
    // Prepare the message address for the mailbox by clearing the lowest 4 bits (ensure alignment)
    // and setting the lowest 4 bits to the channel number (ch).
    unsigned int messageAddress = (((unsigned int)((unsigned long)&mbox) & ~0xF) | (ch & 0xF));
    
    // Wait until the mailbox is not full before writing our message.
    while (*MBOX_STATUS & MBOX_FULL) {
        // Do nothing (NOP) while waiting.
        asm volatile("nop");
    }

    // Write the message address (with channel number) to the mailbox.
    *MBOX_WRITE = messageAddress;

    // Wait for a response to our message.
    while (1) {
        // Wait until the mailbox has something (is not empty).
        while (*MBOX_STATUS & MBOX_EMPTY) {
            // Do nothing (NOP) while waiting.
            asm volatile("nop");
        }

        // Check if the response is for our message.
        if (messageAddress == *MBOX_READ) {
            // Check if the response indicates success.
            return mbox[1] == MBOX_RESPONSE;
        }
    }

    // We should never get here. Return 0 indicating failure as a fallback.
    return 0;
}
