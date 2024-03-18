#include "mini_uart.h"
#include "shell.h"
#include <stddef.h>


void main(void) 
{
	uint32_t a = 0;

	mini_uart_init();
	mini_uart_putln("\nHello Lab 2! It's bootloader!");
	shell();
}