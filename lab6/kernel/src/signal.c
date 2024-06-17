#include <thread.h>
#include <signal.h>
#include <stddef.h>

extern thread* get_current();

void sigkill() {
	kill_thread(get_current() -> id);
}

void test_handler() {
	uart_printf ("This is test handler\r\n");
}

void signal_handler_wrapper(void* func_ptr) {
	// in el0
	uart_printf ("In user mode\r\n");
	void (*func) (void) = func_ptr;
	func();

	// while (1);	

	asm volatile (
		"mov x8, 10;"
		"svc #0;"
	);
}

void handle_signal(void* tf) {
	thread* t = get_current();

	for (int i = 0; i < 10; i ++) {
		if (t -> signal[i] == 0) continue;
		uart_printf ("found signal\r\n");
		t -> signal[i] = 0;
		if (t -> signal_handler[i] == NULL) {
			kill_thread(t -> id);
			uart_printf ("never back\r\n");
		}
		void* func = signal_handler_wrapper;
		uart_printf ("Going to usermode\r\n");
		uart_printf ("%x :(:(:(\r\n", func);
		irq(0);
		t -> sstack_start = my_malloc(4096);
		void* sp = t -> sstack_start + 4096 - 16;
		/*
		asm volatile (
			"mov x0, %0;"
			"bl signal_handler_wrapper;"
			:
			: "r" (i)
			: "x0"
		);
		*/
		asm volatile (
			"mov x1, 0;"
			"msr spsr_el1, x1;"
			"mov x1, %[func];"
			"mov x2, %[sp];"
			"msr elr_el1, x1;"
			"msr sp_el0, x2;"
			"mov x0, %[handler];"
			"mov sp, %[ori_sp];"
			"msr DAIFclr, 0xf;"
			"eret;"
			:
			: [func]    "r" (func), 
			  [sp] 	    "r" (sp),
			  [ori_sp]  "r" (tf),
			  [handler] "r" (t -> signal_handler[i])
			: "x0", "x1", "x2", "sp"
		);
		uart_printf ("should not be here\r\n");
		while (1);
	}
	
}
