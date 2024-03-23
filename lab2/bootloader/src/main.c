#include "uart.h"

int main()
{
	unsigned int size = 0;
	volatile unsigned char *kernel = ((volatile unsigned char *)(0x80000));

	uart_init();
	uart_puts("loading kernel...\n");
	
	for (int i = 0; i < 4; i++)
	{
		unsigned int singal = uart_rev();
		uart_send(singal);
		size |= (singal << 8 * i);
	}

	// read kernel
	while(size--)
		*kernel++ = uart_rev();
	
	uart_puts("kernel-loaded\n");
	asm volatile
	(
		"mov x30, 0x80000\n" 
		"ret\n"
	);

	return 0;
}
