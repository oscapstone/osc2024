#include "uart.h"
#include "mailbox.h"
#include "shell.h"

int main()
{
	uart_init();
	uart_puts("\n");
	get_board_revision();
	get_arm_memory();
	uart_puts("\n");
	simple_shell();
}
