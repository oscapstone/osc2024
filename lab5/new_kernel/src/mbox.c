#include "rpi_mbox.h"
#include "mbox.h"
#include "exception.h"
#include "mini_uart.h"

/* Aligned to 16-byte boundary while we have 28-bits for VC */
volatile unsigned int __attribute__((aligned(16))) pt[64];

int mbox_call(mbox_channel_type channel, unsigned int value)
{	
	print_lock();
	lock();
	unsigned long r = (((unsigned long)((unsigned long)value) & ~0xF) | (channel & 0xF));
	do
	{
		asm volatile("nop");
	} while (*MBOX_STATUS & BCM_ARM_VC_MS_FULL);
			uart_puts("MBOX_STATUS :");
			uart_hex(*MBOX_STATUS);
	*MBOX_WRITE = r;
    uart_puts(" [+] in mbox call [-] ");
    print_lock();
	
	while (1)
	{
		uart_puts("  BCM_ARM_VC_MS_EMPTY : ");
		uart_hex(BCM_ARM_VC_MS_EMPTY);
		do
		{
			asm volatile("nop");
			
		} while (*MBOX_STATUS & BCM_ARM_VC_MS_EMPTY);
		if (r == *MBOX_READ)
		{
			unlock();

			return ((unsigned int *)value)[1] == MBOX_REQUEST_SUCCEED;
		}
	}
	unlock();
    // print_lock();

}