#include "../include/mbox.h"
#include "../include/bcm2837/rpi_mbox.h"

/* Aligned to 16-byte boundary while we have 28-bits for VC */
volatile unsigned int __attribute__((aligned(16))) pt[64];

int mbox_call(mbox_channel_type channel, unsigned int value)
{
	// add channel to lower 4 bit
	value &= ~(0xF);  // get front 28 bits
	value |= channel; // combine with channel

	while ((*MBOX_STATUS & BCM_ARM_VC_MS_FULL) != 0) { } // wait until mail box not full
	*MBOX_WRITE = value;                                 // write to reg

	// read content, check if is equal to the content just wrote
	while (1) {
		while (*MBOX_STATUS & BCM_ARM_VC_MS_EMPTY) { } // wait until mail box not empty
		if (value == *MBOX_READ)                       // read from reg
			return pt[1] == MOBX_REQUEST_SUCCEED;
	}
	return 0;
}