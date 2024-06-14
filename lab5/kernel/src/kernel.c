#include "mini_uart.h"
#include "shell.h"
#include "utils.h"
#include "dtb.h"
#include "timer.h"
#include "exception.h"
#include "memory.h"


void kernel_main(char* arg) {
	char input_buf[MAX_CMD_LEN];

    dtb_init(arg);

	uart_init();
	uart_flush_FIFO();
	uart_interrupt_enable();
	core_timer_enable();

	memory_init();
	irqtask_list_init();
    timer_list_init();

	el1_interrupt_enable();	
	cli_print_welcome_msg();

	while (1) {
		cli_clear_cmd(input_buf, MAX_CMD_LEN);
		uart_puts("▬▬ι═══════- $ ");
		cli_read_cmd(input_buf);
		cli_exec_cmd(input_buf);
	}
}
