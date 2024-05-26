#include "thread.h"
#include "alloc.h"
#include "mini_uart.h"
#include "helper.h"
#include "exception.h"

#define stack_size 4096
#define max_thread_num 500 

thread* thread_start;
thread* thread_tail;

thread** threads;

void** thread_fn;

extern thread* get_current();
extern void switch_to(thread*, thread*);

void thread_init() {
	
	threads = (thread**)my_malloc(sizeof(thread) * max_thread_num);
	thread_fn = (void**)my_malloc(sizeof(void*) * max_thread_num);

	irq(0);
	for (int i = 1; i < max_thread_num; i ++) {
		threads[i] = NULL;
	}

	thread* x = my_malloc(sizeof(thread));
	threads[0] = x;
	x -> id = 0;
	x -> state = 1;
	x -> kstack_start = my_malloc(stack_size);
	x -> sp = x -> kstack_start + stack_size - 16;
	x -> fp = x -> sp;

	x -> stack_start = my_malloc(stack_size);
	asm volatile ("msr tpidr_el1, %0" : "=r" (x));

	thread_start = x;
	thread_tail = thread_start;
	thread_start -> next = NULL;
	thread_start -> prev = NULL;
	irq(1);
}

void kill_zombies() {
	irq(0);
	for (int i = 1; i < max_thread_num; i ++) {
		if (threads[i] -> state == 0) {
			my_free(threads[i] -> stack_start);
			my_free(threads[i]);
			threads[i] = NULL;
		}
	}
	irq(1);
}

void idle() {
	while (1) {
		uart_printf ("In idle\r\n");
		kill_zombies();
		schedule(1);
		// can't be called in el0
		delay(1e7);
	}
}
void im_fine() {
	uart_printf ("I'm fine\r\n");
}

void schedule() {
	if (thread_start -> next == NULL) {
		// uart_printf ("No next thread job\r\n");
		return;
	}
	thread_start = thread_start -> next;
	if (thread_start -> id == get_current() -> id) return;
	
	if (get_current() -> state) {
		thread_tail -> next = get_current();
		get_current() -> prev = thread_tail;
		get_current() -> next = NULL;
		thread_tail = thread_tail -> next;
	}
	thread_start -> prev = NULL;
	uart_printf ("[DEBUG] Scheduled From %d to %d\r\n", get_current() -> id, thread_start -> id);
	// uart_printf ("[DEBUG] LR is at %x\r\n", thread_start -> lr);
	switch_to(get_current(), thread_start);
	// switch_to does the irq(1)
	return;
}

void kill_thread(int id) {
	irq(0);
	uart_printf ("Killing child %d\r\n", id);
	if (thread_tail -> id == id) {
		if (thread_tail == thread_start) {
			uart_printf ("You're killing the last shit and I won't let you do that\r\n");
			return;
		}
		thread_tail = thread_tail -> prev;
	}
	if (threads[id] -> prev != NULL) {
		threads[id] -> prev -> next = threads[id] -> next;
	}
	if (threads[id] -> next != NULL) {
		threads[id] -> next -> prev = threads[id] -> prev;
	}
	threads[id] -> state = 0;
	irq(1);
	if (id == get_current() -> id) { 
		while (1);
	}
	// wait for being scheduled away
	// schedule();
}

trapframe_t* sp;

void thread_func_handler() {
	asm volatile (
		"mov %0, sp;"
		: "=r" (sp)
		:
		: "sp"
	);
	/*
	// uncommenting the upper shit will affect the sp, need to modify them in the ret_from_fork shit
	int id = get_current() -> id;
	void* func = thread_fn[id];
	*/
	uart_printf ("Jumping to %x with sp %x\r\n", thread_fn[get_current() -> id], (char*)sp + 16 * 2);
	asm volatile (
		"add sp, sp, 16 * 2;"
		// "mov x0, sp;"
		// "bl output_sp;"
		:
		:
		: "sp"
	);
	asm volatile ( 
		"mov x0, %0;"
		"blr x0;"
		:
		: "r" (thread_fn[get_current() -> id])
		: "x0", "x30"
	);
	kill_thread(get_current() -> id);
}

int thread_create(void* func) {
	uart_printf ("creating thread\r\n");
	thread* x = my_malloc(sizeof(thread));
	x -> stack_start = my_malloc(stack_size);
	x -> kstack_start = my_malloc(stack_size);
	x -> sp = x -> kstack_start + stack_size - 16; 
	x -> fp = x -> sp;
	x -> lr = thread_func_handler;
	x -> state = 1;

	for (int i = 1; i < max_thread_num; i ++) {
		if (threads[i] == NULL) {
			threads[i] = x;
			thread_fn[i] = func;
			x -> id = i;
			break;
		}
		if (i == max_thread_num - 1) {
			uart_printf ("Not enough thread resources\r\n");
			return -1;
		}
	}
	
	x -> next = NULL; 
	x -> prev = thread_tail;
	thread_tail -> next = x;
	thread_tail = thread_tail -> next;
	uart_printf ("Created %d, prev: %d\r\n", x -> id, x -> prev -> id);
	return x -> id;
}

void foo(){
    for(int i = 0; i < 3; ++i) {
        uart_printf("Thread id: %d %d\n", get_current() -> id, i);
        delay(1e7);
        schedule();
    }
	// kill_thread();
	// uart_printf ("should not come here\r\n");
	// while(1);
}

void thread_test() {
	uart_printf ("started thread testing\r\n");
	thread_create(idle);
	for (int i = 0; i < 3; i ++) {
		thread_create(foo);
	}
	schedule();	
}
