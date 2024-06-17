#include "bcm2837/rpi_mbox.h"
#include "mbox.h"
#include "debug.h"

/* Aligned to 16-byte boundary while we have 28-bits for VC */
volatile unsigned int __attribute__((aligned(16))) pt[64];

int mbox_call(mbox_channel_type channel, unsigned int value)
{
	lock();
	unsigned long r = (((unsigned long)((unsigned long)value) & ~0xF) | (channel & 0xF));
	do
	{
		asm volatile("nop");
	} while (*MBOX_STATUS & BCM_ARM_VC_MS_FULL);
	*MBOX_WRITE = r;
	while (1)
	{
		do
		{
			asm volatile("nop");
		} while (*MBOX_STATUS & BCM_ARM_VC_MS_EMPTY);
		DEBUG("MBOX_WRITE: 0x%x\n", r);
		if (r == *MBOX_READ)
		{
			unlock();
			return ((unsigned int *)value)[1] == MBOX_REQUEST_SUCCEED;
		}
	}
	unlock();
}