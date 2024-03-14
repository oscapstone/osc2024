#include "mini_uart.h"
#include "shell.h"

void kernel_main(void) {
	char input_buf[MAX_CMD_LEN];

	uart_init();
	cli_print_welcome_msg();

	while (1) {
		cli_clear_cmd(input_buf, MAX_CMD_LEN);
		uart_send_string("LAB1(つ´ω`)つ@sh> ");
		cli_read_cmd(input_buf);
		cli_exec_cmd(input_buf);
	}
}
