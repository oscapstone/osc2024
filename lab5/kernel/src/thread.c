#include "thread.h"
#include "alloc.h"
#include "mini_uart.h"

#define stack_size 4096
#define max_thread_num 100

thread* thread_start;
thread* thread_tail;

thread* threads[max_thread_num];

extern thread* get_current();
extern void switch_to(thread*, thread*);

void thread_init() {
	for (int i = 0; i < max_thread_num; i ++) {
		threads[i] = NULL;
	}
	thread_start = my_malloc(sizeof(thread));
	thread_start -> next = NULL;
	thread_start -> prev = NULL;
	thread_tail = thread_start;
	uart_printf ("initialized thread_start and thread_tail\r\n");

	thread* x = my_malloc(sizeof(thread));
	threads[0] = x;
	x -> id = -1;
	x -> state = 1;
	asm volatile ("msr tpidr_el1, %0" : "=r" (x));
}

void kill_zombies() {
	for (int i = 0; i < max_thread_num; i ++) {
		if (threads[i] -> state == 0) {
			my_free(threads[i] -> stack_start);
			my_free(threads[i]);
			threads[i] = NULL;
		}
	}
}

void idle() {
	while (1) {
		uart_printf ("In idle\r\n");
		kill_zombies();
		schedule(1);
	}
}

void schedule(int d) {
	if (thread_start -> next == NULL) {
		uart_printf ("No next thread job, shouldn't happen");
	}
	thread_start = thread_start -> next;
	if (thread_start -> id == -1) {
		thread_start = thread_start -> next;
	}
	if (thread_start -> id == get_current() -> id) return;
	
	if (d) {
		thread_tail -> next = get_current();
		get_current() -> prev = thread_tail;
		get_current() -> next = NULL;
		thread_tail = thread_tail -> next;
	}
		
	switch_to(get_current(), thread_start);
}

void thread_create(void* func) {
	thread* x = my_malloc(sizeof(thread));
	x -> stack_start = my_malloc(stack_size);
	x -> sp = x -> stack_start + stack_size - 16; 
	x -> fp = x -> sp;
	x -> lr = func; // by this when ret, it'll do function.
	x -> state = 1;

	for (int i = 0; i < max_thread_num; i ++) {
		if (threads[i] == NULL) {
			threads[i] = x;
			x -> id = i;
			break;
		}
		if (i == max_thread_num - 1) {
			uart_printf ("Not enough thread resources\r\n");
			return;
		}
	}
	
	x -> next = NULL; 
	x -> prev = thread_tail;
	thread_tail -> next = x;
	thread_tail = thread_tail -> next;
}

void kill_thread() {
	get_current() -> prev -> next = get_current() -> next;
	get_current() -> next -> prev = get_current() -> prev;
	get_current() -> state = 0;
	schedule(0);
}

void delay(int t) {
	while (t --);
}

void foo(){
    for(int i = 0; i < 3; ++i) {
        uart_printf("Thread id: %d %d\n", get_current() -> id, i);
        delay(1000000);
        schedule(1);
    }
	kill_thread();
	uart_printf ("should not come here\r\n");
	while(1);
}

void thread_test() {
	uart_printf ("started thread testing\r\n");
	thread_create(idle);
	for (int i = 0; i < 3; i ++) {
		thread_create(foo);
	}
	schedule(0);	
}
