#include "thread.h"
#include "alloc.h"
#include "mini_uart.h"
#include "helper.h"
#include "exception.h"

#define stack_size 4096
#define max_thread_num 100

thread* thread_start;
thread* thread_tail;

thread* threads[max_thread_num];

void* thread_fn[max_thread_num];

extern thread* get_current();
extern void switch_to(thread*, thread*);

void thread_init() {
	for (int i = 1; i < max_thread_num; i ++) {
		threads[i] = NULL;
	}

	thread* x = my_malloc(sizeof(thread));
	threads[0] = x;
	x -> id = 0;
	x -> state = 1;
	x -> stack_start = my_malloc(stack_size);
	x -> sp = x -> stack_start + stack_size - 16;
	x -> fp = x -> sp;
	asm volatile ("msr tpidr_el1, %0" : "=r" (x));

	thread_start = x;
	thread_tail = thread_start;
	thread_start -> next = NULL;
	thread_start -> prev = NULL;
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

void schedule(int d) {
	irq(0);
	if (thread_start -> next == NULL) {
		uart_printf ("No next thread job\r\n");
		return;
	}
	thread_start = thread_start -> next;
	if (thread_start -> id == get_current() -> id) return;
	
	if (d) {
		thread_tail -> next = get_current();
		get_current() -> prev = thread_tail;
		get_current() -> next = NULL;
		thread_tail = thread_tail -> next;
	}
	thread_start -> prev = NULL;
	uart_printf ("Scheduled From %d to %d\r\n", get_current() -> id, thread_start -> id);
	switch_to(get_current(), thread_start);
	return;
}

void kill_thread(int id) {
	irq(0);
	uart_printf ("pid %d is being killed\r\n", id);
	if (threads[id] -> prev != NULL) {
		threads[id] -> prev -> next = threads[id] -> next;
	}
	if (threads[id] -> next != NULL) {
		threads[id] -> next -> prev = threads[id] -> prev;
	}

	uart_printf ("now: %d, thread_start: %d, next %d\r\n", get_current() -> id, thread_start -> id, thread_start -> next -> id);
	threads[id] -> state = 0;
	irq(1);
	schedule(0);
}

void thread_func_handler() {
	trapframe_t* sp;
	asm volatile (
		"mov %0, sp;"
		: "=r" (sp)
		:
		: "sp"
	);
	int id = get_current() -> id;
	void* func = thread_fn[id];
	asm volatile ( 
		"mov x0, %0;"
		"blr x0;"
		:
		: "r" (func)
		: "x0", "x30"
	);
	uart_printf ("Something is ended\r\n");
	kill_thread(id);
}

int thread_create(void* func) {
	thread* x = my_malloc(sizeof(thread));
	x -> stack_start = my_malloc(stack_size);
	x -> sp = x -> stack_start + stack_size - 16; 
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

	uart_printf ("created %d, prev is %d\r\n", x -> id, x -> prev -> id);
		
	return x -> id;
}

void foo(){
    for(int i = 0; i < 3; ++i) {
        uart_printf("Thread id: %d %d\n", get_current() -> id, i);
        delay(1e7);
        schedule(1);
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
	schedule(0);	
}
