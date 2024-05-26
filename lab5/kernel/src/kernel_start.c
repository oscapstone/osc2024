#include <mini_uart.h>
#include <shell.h>
#include "fdt.h"
#include "cpio.h"
#include "alloc.h"
#include "thread.h"

extern char _bootloader[];
extern void core_timer_enable();
extern char* fdt_addr;

void kernel_begin(char* fdt) {
	fdt_addr = fdt;
	uart_init();

	fdt_traverse(get_initramfs_addr); // also set _cpio_file
	manage_init();
	// core_timer_enable();

	thread_init();

	shell_begin(fdt);
	return;
}
