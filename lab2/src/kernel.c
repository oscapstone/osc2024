#include "mini_uart.h"
#include "shell.h"
#include "string_utils.h"

void kernel_main(void)
{
	uart_init();
	uart_send_string("Reboot finished!!!\r\n");
	shell();

	// char *str = "0000000B";
	// unsigned int result = hexstr2val(str, 8);
	// uart_hex(result);
}