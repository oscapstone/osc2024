#include "../include/mini_uart.h"
#include "../include/utils.h"

void *start_addr = (void *)0x80000;

void main(void)
{
	uart_init();
	uart_send_string("Fuck you!!!\r\n");

	/* load size */
	unsigned int size = 0;
	unsigned char *ptr_size = (unsigned char *)&size;
	for (int i = 0; i < 4; i++) {
		ptr_size[i] = uart_recv();
	}
	
	/* send string to tell python to write kernel image */
	uart_send_string("Loading...\r\n");
	unsigned char *curr_ptr = (unsigned char *)start_addr;
	while (size --) {
		*curr_ptr++ = uart_recv();
	}
	/* send string to tell python to end */
	uart_send_string("Kernel loaded!!!\r\n");

	/* restore arguments and jump to the new kernel */
    asm volatile (
        "mov x0, x10;"
        "mov x1, x11;"
        "mov x2, x12;"
        "mov x3, x13;"
        /* we must force an absolute address to branch to */
        "mov x30, 0x80000; ret"
    );
}