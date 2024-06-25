#include "mini_uart.h"
#include "utils.h"
#include "shell.h"


void main(void)
{
	uart_init();
	shell();
}
