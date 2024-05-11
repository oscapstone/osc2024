#include "thread.h"
#include "alloc.h"
#include "mini_uart.h"

#define stack_size 4096
#define max_thread_num 100

thread_node* thread_start;
thread_node* thread_tail;

thread_node* threads[max_thread_num];

extern thread_node* get_current();
extern void switch_to(thread_node*, thread_node*);

void thread_init() {
	for (int i = 0; i < max_thread_num; i ++) {
		threads[i] = NULL;
	}
	thread_start = my_malloc(sizeof(thread_node));
	thread_start -> next = NULL;
	thread_tail = thread_start;
	uart_printf ("initialized thread_start and thread_tail\r\n");

	thread* x = my_malloc(sizeof(thread));
	thread_node* y = my_malloc(sizeof(thread_node));
	y -> thread = x;
	threads[0] = y;
	y -> id = 0;
	asm volatile ("msr tpidr_el1, %0" : "=r" (y));
}

void kill_zombies() {
	for (int i = 0; i < max_thread_num; i ++) {
		if (threads[i] -> thread -> state == 0) {
			my_free(threads[i] -> thread -> stack_start);
			my_free(threads[i] -> thread);
			my_free(threads[i]);
			threads[i] = NULL;
		}
	}
}

void idle() {
	uart_printf ("In idle\r\n");
	while (1) {
		kill_zombies();
		schedule();
	}
}

void schedule() {
	if (thread_start -> next == NULL) {
		return;
	}
	
	thread_start = thread_start -> next;
	uart_printf("jumping from %d to %d\r\n", get_current() -> id, thread_start -> id);
	uart_printf ("%x\r\n", thread_start -> thread -> lr);
	switch_to(get_current(), thread_start);
}

void thread_create(void* func) {
	thread* x = my_malloc(sizeof(thread));
	x -> stack_start = my_malloc(stack_size);
	x -> sp = x -> stack_start + stack_size - 16; 
	x -> fp = x -> sp;
	x -> lr = func; // by this when ret, it'll do function.
	x -> state = 1;

	thread_node* y = my_malloc(sizeof(thread_node));
	y -> thread = x;
	
	for (int i = 0; i < max_thread_num; i ++) {
		if (threads[i] == NULL) {
			threads[i] = y;
			y -> id = i;
			break;
		}
		if (i == max_thread_num - 1) {
			uart_printf ("Not enough thread resources\r\n");
			return;
		}
	}
	
	y -> next = thread_tail -> next;
	thread_tail -> next = y;
	thread_tail = y;

	uart_printf ("%d: %x\r\n", y -> id, x -> lr);
}

void delay(int t) {
	while (t --);
}

void foo(){
    for(int i = 0; i < 10; ++i) {
        uart_printf("Thread id: %d %d\n", get_current() -> id, i);
        delay(1000000);
        schedule();
    }
	get_current() -> thread -> state = 0;
}

void thread_test() {
	uart_printf ("started thread testing\r\n");
	thread_create(idle);
	for (int i = 0; i < 3; i ++) {
		thread_create(foo);
	}
	schedule();	
}
