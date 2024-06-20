#include "mini_uart.h"
#include "shell.h"

void kernel_main(void)
{
	uart_init();
	uart_send_string("\r\nthis device is enabled\r\n");
	uart_send_string("\r\nrunning bootloader\r\n");

	while (1) {
		shell();
	}
}
