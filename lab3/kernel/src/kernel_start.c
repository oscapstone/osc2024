#include <mini_uart.h>
#include <shell.h>
#include "fdt.h"
#include "cpio.h"

extern char _bootloader[];

void kernel_begin(char* fdt) {
	uart_init();
	uart_printf("This is the real kernel\n");

	char t[100];
	uart_recv_string(t);

	fdt_traverse(get_initramfs_addr, fdt);
	parse_cpio();

	shell_begin(fdt);
	return;
}
