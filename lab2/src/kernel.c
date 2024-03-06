#include "mini_uart.h"
#include "shell.h"
#include "devicetree.h"

void kernel_main(fdt_header *devicetree_ptr) {
	set_devicetree_addr(devicetree_ptr);
	uart_init();
	uart_printf("\nHello World!\n");
	shell_loop();
}