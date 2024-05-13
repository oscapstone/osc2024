
#include "base.h"
#include "io/dtb.h"
#include "io/uart.h"
#include "shell/shell.h"
#include "utils/utils.h"
#include "utils/printf.h"
#include "peripherals/irq.h"
#include "io/exception.h"
#include "peripherals/timer.h"
#include "mm/mm.h"
#include "proc/task.h"

void putc(void *p, char c) {
	if (c == '\n') {
		uart_send_char('\r');
	}
	uart_send_char(c);
}

void func_task_a() {
	shell();
}

void func_task_b() {
	for (;;) {
		U64 flag = utils_read_sysreg(DAIF);
		printf("B: DAIF: %x\n", flag);
	}
}

void func_task_c() {
	for (;;) {
		U64 flag = utils_read_sysreg(DAIF);
		printf("C: DAIF: %x\n", flag);
	}
}

void main() {

    // initialze UART
    uart_init();

	init_printf(0, putc);

	// init memory management	
	mm_init();

	set_exception_vector_table();
	enable_interrupt_controller();

	timer_init();

	task_init();

	// enabling the interrupt
	enable_interrupt();
	//TASK* task_a = task_create(&func_task_a);
	//NS_DPRINT("task a pid = %d\n", task_a->pid);
	//task_run(task_a);
	TASK* task_b = task_create(&func_task_b);
	NS_DPRINT("task b pid = %d\n", task_b->pid);
	task_b->priority = 3;
	task_run(task_b);
	TASK* task_c = task_create(&func_task_c);
	NS_DPRINT("task a pid = %d\n", task_c->pid);
	task_run(task_c);

	// init idle
	while (1) {
		printf("A\n");
		task_schedule();
	}

}
