#include <mini_uart.h>
#include <shell.h>
#include "fdt.h"

extern char _bootloader[];

void kernel_begin(char* fdt) {
	uart_init();
	uart_printf("This is the real kernel\n");
	
	fdt_traverse(get_initramfs_addr, fdt);
	shell_begin(fdt);
	
	return;
}
