
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
#include "lib/fork.h"
#include "lib/getpid.h"
#include "fs/fs.h"

// in exception.S
void set_exception_vector_table();
// in shell.c
void get_cpio_addr(int token, const char* name, const void *data, unsigned int size);

void putc(void *p, char c) {
	if (c == '\n') {
		uart_send_char('\r');
	}
	uart_send_char(c);
}

void func_task_a() {
	shell();

	task_exit(0);
}

void func_task_b() {
	for (;;) {
		asm volatile("mov		x8, 0\n\t"
					"svc 0\n\t");
	}
	// int pid = fork();
	// if (pid == 0) {
	// 	printf("child process");
	// } else {
	// 	printf("parent process, child pid = %d\n", pid);
	// }
	// // the bug that doesn't show the child process correctly is because we didn't copy the whole function to a new space
	// // the the memory of pid is the same as parent process
	// printf("end task pid = %d\n", getpid());
	// task_exit();
}

void func_task_c() {
	for (int i = 0;i < 10; i++) {
		printf("C . %d\n", i);
		task_schedule();
	}
	task_exit(0);
}

void main() {

    // initialze UART
    uart_init();

	init_printf(0, putc);

	// init memory management	
	mm_init();

	// setup the kernel space mapping
	setup_kernel_space_mapping();

	set_exception_vector_table();
	enable_interrupt_controller();

	timer_init();

	// getting CPIO addr
	fdt_traverse(get_cpio_addr);

	// initializing file system
	fs_init();

	task_init();

	// enabling the interrupt
	enable_interrupt();
	TASK* task_a = task_create_kernel("kernel_tty", TASK_FLAGS_KERNEL);
	task_a->cpu_regs.lr = (U64)func_task_a;
	task_run(task_a);
	// TASK* task_b = task_create_user("task_b", NULL);
	// NS_DPRINT("task b pid = %d\n", task_b->pid);
	// task_copy_program(task_b, func_task_b, (U64)&func_task_c - (U64)&func_task_b);
	// task_run_to_el0(task_b);
	// TASK* task_c = task_create("task_c", TASK_FLAGS_KERNEL, &func_task_c);
	// NS_DPRINT("task c pid = %d\n", task_c->pid);
	// task_run(task_c);

	// init idle
	while (1) {
		//printf("a\n");
		task_kill_dead();
		task_schedule();
	}

}
