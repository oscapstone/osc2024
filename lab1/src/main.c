#include "../include/shell.h"
#include "../include/uart1.h"

void main()
{
	char input_buffer[CMD_MAX_LEN];

	uart_init();
	cli_print_banner();

	while (1) {
		cli_cmd_clear(input_buffer, CMD_MAX_LEN);
		uart_puts("# ");
		cli_cmd_read(input_buffer);
		cli_cmd_exec(input_buffer);
	}
}