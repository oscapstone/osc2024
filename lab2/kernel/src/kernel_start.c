#include <mini_uart.h>
#include <shell.h>

extern char _bootloader[];

typedef void (*kernel_funcp)();

void kernel_begin() {
	uart_init();
	uart_printf("This is the real kernel\n");
	shell_begin();

	// the real kernel ends, jumps back to bootloader.
	((kernel_funcp)_bootloader)();

	return;
}
