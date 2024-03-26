#include "mini_uart.h"
#include "shell.h"
#include "peripherals/devicetree.h"
#include "utils.h"

void kernel_main(fdt_header *devicetree_ptr) {
	
	set_devicetree_addr(devicetree_ptr);
	uart_init();
	uart_printf("\nHello World!\n");
	uart_printf("Exception level: %d\n", get_el());
	shell_loop();
}