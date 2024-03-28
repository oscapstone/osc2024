#include "mini_uart.h"
#include "shell.h"
#include "devicetree.h"

void kernel_main(char *addr) {
	devicetree_addr = addr;
	uart_init();
	uart_printf("\nThis is 3rd bootloader. Hello World!\n");
	shell_loop();
}