#include "uart.h"
#include "shell.h"

int main()
{
	uart_init();
	uart_puts("Welcome!\n");
	run_shell();
	return 0;
}