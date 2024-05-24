#include "mini_uart.h"
#include "shell.h"
#include "utils.h"
#include "dtb.h"
#include "timer.h"
#include "exception.h"

extern char* dtb_ptr;

void kernel_main(char* arg) {
	char input_buf[MAX_CMD_LEN];

	dtb_ptr = arg;
    parse_dtb_tree(dtb_ptr, dtb_callback_initramfs);

	uart_init();
	irqtask_list_init();
    timer_list_init();

	uart_flush_FIFO();
	uart_interrupt_enable();

	core_timer_enable();
	el1_interrupt_enable();
	
	cli_print_welcome_msg();

	while (1) {
		cli_clear_cmd(input_buf, MAX_CMD_LEN);
		uart_puts("▬▬ι═══════- $ ");
		cli_read_cmd(input_buf);
		cli_exec_cmd(input_buf);
	}
}
