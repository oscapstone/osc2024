#include "mini_uart.h"
#include "shell.h"
#include"stdio.h"
#include"dtb.h"
void kernel_main(void* x0){
	_dtb_addr=x0;
	fdt_traverse(initramfs_callback);
	uart_init();
	uart_send_string("\r\nthis device is enabled\r\n");

	while (1) {
		shell();
	}
}
