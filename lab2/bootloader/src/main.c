#include "mini_uart.h"
#include "shell.h"

void bootloader_main()
{
	uart_init();
	uart_send_string("\r\n");
	shell();
}
