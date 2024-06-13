#include "mini_uart.h"
#include "shell.h"
#include "device_tree.h"
#include "cpio.h"

void kernel_main(void)
{
	uart_init();
	uart_send_string("\r\nkernel start\r\n");

	dev_tree_parser(initramfs_callback);

	shell();
}
