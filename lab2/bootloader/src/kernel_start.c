#include <mini_uart.h>
#include <shell.h>

void kernel_begin() {
	uart_init();
	uart_printf("This is the bootloader\n");
	shell_begin();

	return;
}
