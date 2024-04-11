#include <mini_uart.h>
#include <shell.h>

extern char _start[];
extern char _end[];
extern char _bootloader[];

void kernel_begin(char* fdt) {


	uart_init();	
	uart_printf("This is the reloacted bootloader\n");
	shell_begin(fdt);

	return;
}
