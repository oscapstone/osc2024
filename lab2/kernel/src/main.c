#include "uart.h"
#include "mailbox.h"
#include "shell.h"
#include "cpio.h"

int main()
{
	uart_init();
	build_file_arr();
	uart_puts("\n");
	get_board_revision();
	get_arm_memory();
	uart_puts("\n");
	simple_shell();

	return 0;
}
