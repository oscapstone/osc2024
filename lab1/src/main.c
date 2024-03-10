#include "uart.h"
#include "shell.h"

void main() {
	
	uart_init();
	uart_send_string("Type in 'help' to get instruction menu!\n");
	
	shell();
	
	return;
}
