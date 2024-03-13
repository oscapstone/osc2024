#include "rpi_gpio.h"
#include "mbox.h"

/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[36];

#define VIDEOCORE_MBOX  (PERIPHERAL_BASE+0x0000B880)
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


#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY      0x00010005

#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001

#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

int get_board_revision(unsigned int *board_revision){
  mbox[0] = 7 * 4;               // buffer size in bytes
  mbox[1] = REQUEST_CODE;
  
  /* tags begin */
  mbox[2] = GET_BOARD_REVISION;  // tag identifier
  mbox[3] = 4;                   // maximum of request and response value buffer's length.
  mbox[4] = TAG_REQUEST_CODE;
  mbox[5] = 0;                   // value buffer
  mbox[6] = END_TAG;
  /* tags end */

  mbox_call(MBOX_CH_PROP);

  // it should be 0xa020d3 for rpi3 b+
  *board_revision = mbox[5];

  return 0;
}

int get_arm_memory_info(unsigned int *base_address, unsigned int *size){
  mbox[0] = 8 * 4;               // buffer size in bytes
  mbox[1] = REQUEST_CODE;
  
  /* tags begin */
  mbox[2] = GET_ARM_MEMORY;      // tag identifier
  mbox[3] = 8;                   // maximum of request and response value buffer's length.
  mbox[4] = TAG_REQUEST_CODE;
  mbox[5] = 0;
  mbox[6] = 0;                   // value buffer
  mbox[7] = END_TAG;
  /* tags end */


  mbox_call(MBOX_CH_PROP);       // message passing procedure call, you should implement it following the 6 steps provided above.
  
  *base_address = mbox[5];
  *size = mbox[6];

  return 0 ;
}