#include "mini_uart.h"
#include "shell.h"
#include "utils.h"
#include "dtb.h"

extern char* dtb_ptr;

void kernel_main(char* arg) {
	char input_buf[MAX_CMD_LEN];

	dtb_ptr = arg;
    parse_dtb_tree(dtb_ptr, dtb_callback_initramfs);

	uart_init();
	cli_print_welcome_msg();
	uart_puts("[DTB loaded from: 0x%x]\r\n\r\n", arg);

	while (1) {
		cli_clear_cmd(input_buf, MAX_CMD_LEN);
		uart_puts("LAB2(つ´ω`)つ@sh> ");
		cli_read_cmd(input_buf);
		cli_exec_cmd(input_buf);
	}
}
