#include "uart.h"

int main()
{
	uart_init();
	uart_puts("UART Bootloader\n");
	uart_puts("Loading the kernel image...\n");

	// Load kernel image
	unsigned int size = 0;
	unsigned char *buf = (unsigned char *)&size;
	for (int i = 0; i < 4; i++) {
		buf[i] = uart_getc();
	}

	uart_puts("size ok\n");

	char *kernel = (char *)0x80000;
	while (size--)
		*kernel++ = uart_getc();

	uart_puts("kernel ok\n");

	asm volatile("mov x30, 0x80000; ret;");
	return 0;
}