#include <mini_uart.h>
#include <shell.h>
#include "fdt.h"
#include "cpio.h"
// #include "alloc.h"

extern char _bootloader[];
extern void core_timer_enable();

void kernel_begin(char* fdt) {
	uart_init();

	fdt_traverse(get_initramfs_addr, fdt); // also set _cpio_file
	manage_init(fdt);
	core_timer_enable();
	shell_begin(fdt);
	return;
}
