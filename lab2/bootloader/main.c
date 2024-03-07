#include "uart.h"
#include "utils.h"

int main()
{
	uart_init();
	uart_puts("UART Bootloader\n");

	// Get kernel image size
	char buf[16] = { 0 };
	for (int i = 0; i < 16; i++) {
		buf[i] = uart_getc();
		if (buf[i] == '\n') {
			buf[i] = '\0';
			break;
		}
	}

	uart_puts("Kernel size: ");
	uart_puts(buf);
	uart_puts(" bytes\n");

	// Load kernel image
	uart_puts("Loading the kernel image...\n");
	unsigned int size = atoi(buf);
	char *kernel = (char *)0x80000;
	while (size--)
		*kernel++ = uart_getc();
	asm volatile("nop;"
		     "mov x0, x10;"
		     "mov x1, x11;"
		     "mov x2, x12;"
		     "mov x3, x13;"
		     "mov x30, 0x80000; ret;"); // Jump to the new kernel

	return 0;
}