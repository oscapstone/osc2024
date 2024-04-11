#include "mini_uart.h"
#include "shell.h"
#include "alloc.h"
#include "init_ramdisk.h"
#include "fdt.h"


void kernel_main(void *dtb_addr)
{
	uart_init();
	allocator_init();

	fdt_traverse(init_ramdisk_callback, dtb_addr);

	while (1) {
		shell();
	}
}
