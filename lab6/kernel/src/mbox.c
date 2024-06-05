#include "bcm2837/rpi_mbox.h"
#include "mbox.h"
#include "debug.h"
#include "stdint.h"

/* Aligned to 16-byte boundary while we have 28-bits for VC */
volatile unsigned int __attribute__((aligned(16))) pt[64];

int mbox_call(mbox_channel_type channel, uint64_t value)
{
	// kernel_lock_interrupt();
	// unsigned long r = (((unsigned long)((unsigned long)value) & ~0xF) | (channel & 0xF));
	// do
	// {
	// 	asm volatile("nop");
	// } while (*MBOX_STATUS & BCM_ARM_VC_MS_FULL);
	// *MBOX_WRITE = r;
	// while (1)
	// {
	// 	do
	// 	{
	// 		asm volatile("nop");
	// 	} while (*MBOX_STATUS & BCM_ARM_VC_MS_EMPTY);
	// 	DEBUG("MBOX_WRITE: 0x%x\n", r);
	// 	if (r == *MBOX_READ)
	// 	{
	// 		DEBUG("MBOX_READ: 0x%x\n", r);;
	// 		kernel_unlock_interrupt();
	// 		return ((unsigned int *)value)[1] == MBOX_REQUEST_SUCCEED;
	// 	}
	// 	DEBUG("ERROR: MBOX_READ: 0x%x\n", *MBOX_READ);
	// }
	// DEBUG("MBOX out of while\n");
	// kernel_unlock_interrupt();


	
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
            return pt[1] == MBOX_REQUEST_SUCCEED;
    }
    return 0;
}