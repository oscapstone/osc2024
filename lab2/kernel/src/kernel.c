#include "mini_uart.h"
#include "shell.h"
#include "utils.h"

void kernel_main(char* arg) {
	char input_buf[MAX_CMD_LEN];

	uart_init();
	cli_print_welcome_msg();

	while (1) {
		cli_clear_cmd(input_buf, MAX_CMD_LEN);
		uart_puts("LAB2(つ´ω`)つ@sh> ");
		cli_read_cmd(input_buf);
		cli_exec_cmd(input_buf);
	}
}
