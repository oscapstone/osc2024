#include "mini_uart.h"

void main(void) 
{
	mini_uart_init();
	mini_uart_putln("\nHello Lab 2! It's kernel!");

	while (1) {}
}