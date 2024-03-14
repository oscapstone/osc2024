#include "gpio.h"
#include "mbox.h"

/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[36];

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
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

int get_board_revision() {

    mbox[0] = 7*4; // buffer size in bytes
    mbox[1] = MBOX_REQUEST; // this is a request message
    // tags begin
    mbox[2] = MBOX_TAG_GETBRDREV; // tag identifier
    mbox[3] = 4; // maximum of request and response value buffer's length.
    mbox[4] = 0; // request code
    mbox[5] = 0; // value buffer
    // tags end
    mbox[6] = MBOX_TAG_LAST;

    return mbox_call(MBOX_CH_PROP);
}

int get_arm_memory() {

    mbox[0] = 8*4; // buffer size in bytes
    mbox[1] = MBOX_REQUEST; // this is a request message
    // tags begin
    mbox[2] = MBOX_TAG_GETARMMEM; // tag identifier
    mbox[3] = 8; // maximum of request and response value buffer's length.
    mbox[4] = 0; // request code
    mbox[5] = 0; // value buffer(base address)
    mbox[6] = 0; // value buffer(size)
    // tags end
    mbox[7] = MBOX_TAG_LAST;

    return mbox_call(MBOX_CH_PROP);
}