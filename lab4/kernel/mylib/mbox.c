#include "gpio.h"

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

#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY      0x00010005
#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch)
{
    // ch & 0xf setting channel
    // &~0xF send setting of mbox
    // we use 8 (cpu -> gpu)
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

void get_board_revision(){

    mbox[0] = 7 * 4; // buffer size in bytes
    mbox[1] = REQUEST_CODE;
    // tags begin
    mbox[2] = GET_BOARD_REVISION; // tag identifier
    mbox[3] = 4; // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0; // value buffer
    // tags end
    mbox[6] = END_TAG;
    // channel 8 is cpu -> gpu
    mbox_call(8); // message passing procedure call, you should implement it following the 6 steps provided above.
    uart_puts("Board Revision: 0x");
    uart_hex(mbox[5]);// it should be 0xa020d3 for rpi3 b+
    uart_puts("\n");
}

void get_ARM_memory() {
     // get the board's unique serial number with a mailbox call

    mbox[0] = 8 * 4;                
    mbox[1] = REQUEST_CODE;         // this is a request message
    // tags begin
    mbox[2] = GET_ARM_MEMORY;   
    mbox[3] = 8;                    // buffer size
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;                    // base address
    mbox[6] = 0;                    // size in bytes
    // tags end
    mbox[7] = END_TAG;
    // channel 8 is cpu -> gpu
    mbox_call(8);
    // send the message to the GPU and receive answer
    
    uart_puts("ARM memory base address: 0x");
    uart_hex(mbox[5]);
    uart_puts("\n");

    uart_puts("ARM memory size: 0x");
    uart_hex(mbox[6]);
    uart_puts("\n");
    
}

void mbox_info() {
     
    get_board_revision();
    get_ARM_memory();
   
}