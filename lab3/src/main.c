#include "mini_uart.h"
#include "shell.h"
#include "alloc.h"
#include "init_ramdisk.h"
#include "fdt.h"


void kernel_main(void *dtb_addr)
{
	uart_init();
	uart_enable_interrupt();
	allocator_init();
	uart_send_string("\r\n\r\ndtb_addr: ");
	uart_send_string_int2hex((unsigned long)dtb_addr);
	uart_send_string("\r\n");

	fdt_traverse(init_ramdisk_callback, dtb_addr);

	while (1) {
		shell();
	}
}
