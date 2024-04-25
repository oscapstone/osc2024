#include "mini_uart.h"
#include "shell.h"
#include "utils.h"
#include "dtb.h"
#include "timer.h"
#include "exception.h"
#include "memory.h"
#include "sched.h"

extern thread_t *curr_thread;
extern thread_t *threads[];

void kernel_main(char* arg) {
    dtb_init(arg);

	uart_init();
	uart_flush_FIFO();
	uart_interrupt_enable();
	core_timer_enable();

	memory_init();
	irqtask_list_init();
    timer_list_init();

	init_thread_sched();
	el1_interrupt_enable();
	
	load_context(&curr_thread->context); // jump to idle thread and unlock interrupt
}
