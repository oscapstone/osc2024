
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
	task_exit();
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

	task_init();

	// enabling the interrupt
	enable_interrupt();
	// TASK* task_a = task_create("task_a", TASK_FLAGS_KERNEL, &func_task_a);
	// NS_DPRINT("task a pid = %d\n", task_a->pid);
	// //task_assign_vma_region(task_a, 0, 0x100000, VMA_FLAGS_EXEC | VMA_FLAGS_READ | VMA_FLAGS_WRITE);
	// //mem_map_page(task_a, 0, kzalloc(PD_PAGE_SIZE));
	// task_run(task_a);
	TASK* task_b = task_create("task_b", NULL);
	NS_DPRINT("task b pid = %d\n", task_b->pid);
	task_copy_program(task_b, &func_task_b, (char*)&func_task_c - (char*)&func_task_b);
	task_run(task_b);
	// TASK* task_c = task_create("task_c", TASK_FLAGS_KERNEL, &func_task_c);
	// NS_DPRINT("task c pid = %d\n", task_c->pid);
	// task_run(task_c);

	// init idle
	while (1) {
		//printf("A\n");
		task_kill_dead();
		task_schedule();
	}

}
