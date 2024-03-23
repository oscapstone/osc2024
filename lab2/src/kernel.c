#include "../include/mini_uart.h"
#include "../include/shell.h"
#include "../include/string_utils.h"
#include "../include/mem_utils.h"

void kernel_main(void)
{
	uart_init();
	uart_send_string("Reboot finished!!!\r\n");
	char *tmp = malloc(4);
	if (!tmp) {
		uart_send_string("HEAP overflow!!!\r\n");
		return;
	}
	tmp[0] = 'F';
	tmp[1] = 'U';
	tmp[2] = 'K';
	tmp[3] = '\0';
	uart_send_string(tmp);
	uart_send_string("\r\n");
	shell();

	// char *str = "0000000B";
	// unsigned int result = hexstr2val(str, 8);
	// uart_hex(result);
}