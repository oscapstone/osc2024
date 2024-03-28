#include "mini_uart.h"
#include "shell.h"

void kernel_main(void) {
	uart_init();
	uart_printf("\nHello World!\n");
	shell_loop();
}