#include "uart.h"
#include "shell.h"

void main() {
	
	uart_init();
	
	//while (1) {uart_send_string("test\r\n");}

	shell();
	
	return;
}
