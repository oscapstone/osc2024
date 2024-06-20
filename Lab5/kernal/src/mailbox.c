#include"mailbox.h"
#include"peripherals/mbox_base.h"
/* Aligned to 16-byte boundary while we have 28-bits for VC */
volatile unsigned int  __attribute__((aligned(16))) pt[64];
 
int mbox_call( mbox_channel_type channel, unsigned int value )
{
    // Add channel to lower 4 bit
    value &= ~(0xF);
    value |= channel;
    while ( (*MBOX_STATUS & BCM_ARM_VC_MS_FULL) ) {}
    // Write to Register
    *MBOX_WRITE = value;
    while(1) {
        while ( *MBOX_STATUS & BCM_ARM_VC_MS_EMPTY ) {}
        // Read from Register
        if (value == *MBOX_READ)
            return pt[1] == MBOX_REQUEST_SUCCEED;
    }
    return 0;
}

int mbox_call_buf( mbox_channel_type channel,unsigned int buf)
{
    // Add channel to lower 4 bit
    unsigned int value=(unsigned int)buf;
    value &= ~(0xF);
    value |= channel;
    while ( (*MBOX_STATUS & BCM_ARM_VC_MS_FULL) ) {}
    // Write to Register
    *MBOX_WRITE = value;
    while(1) {
        while ( *MBOX_STATUS & BCM_ARM_VC_MS_EMPTY ) {}
        // Read from Register
        if (value == *MBOX_READ){
            unsigned int* buff=(char*)buf;
            return buff[1] == MBOX_REQUEST_SUCCEED;
        }
    }
    return 0;
}

 
