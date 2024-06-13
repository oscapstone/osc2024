#include "mini_uart.h"
#include "shell.h"
#include "device_tree.h"
#include "cpio.h"
#include "timer.h"
#include "exception.h"

void kernel_main(void)
{
	uart_init();
	async_uart_init();

	#ifndef QEMU
	dev_tree_parser(initramfs_callback);
	#endif

	core_timer_enable();
	set_time_out_cmp(60000);
	uart_send_string("\r\nkernel start\r\n");
	
	shell();
}
