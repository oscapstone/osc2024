#include "bcm2837/rpi_mbox.h"
#include "mbox.h"

/* Aligned to 16-byte boundary while we have 28-bits for VC */
volatile unsigned int  __attribute__((aligned(16))) m[64];

int mbox_call( mbox_channel_type channel, unsigned int value )
{
    // Add channel to lower 4 bit
    value &= ~(0xF);
    value |= channel;
    while ( (*MBOX_STATUS & BCM_ARM_VC_MS_FULL) != 0 ) {}
    // Write to Register
    *MBOX_WRITE = value;
    while(1) {
        while ( *MBOX_STATUS & BCM_ARM_VC_MS_EMPTY ) {}
        // Read from Register
        if (value == *MBOX_READ)
            return m[1] == MBOX_REQUEST_SUCCEED;
    }
    return 0;
}