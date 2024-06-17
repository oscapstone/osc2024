#include "thread.h"
#include "alloc.h"
#include "mini_uart.h"
#include "helper.h"
#include "exception.h"
#include "signal.h"
#include "mmu.h"
#include "utils.h"
#include "system_call.h"

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

	for (int i = 0; i < max_thread_num; i ++) {
		threads[i] = NULL;
	}

	thread* x = my_malloc(sizeof(thread));
	threads[0] = x;
	x -> id = 0;
	x -> state = 1;
	x -> kstack_start = my_malloc(stack_size);
	x -> sp = x -> kstack_start + stack_size - 16;
	x -> fp = x -> sp;
	
	x -> stack_start = my_malloc(stack_size * 4);

	for (int i = 0; i < 16; i ++) {
		x -> fds[i] = NULL;
	}
	x -> cwd[0] = '/';
	x -> cwd[1] = '\0';

	long* PGD = my_malloc(4096);
	for (int i = 0; i < 4096; i ++) {
		((char*)PGD)[i] = 0;
	}
	x -> PGD = va2pa(PGD);

	for (int i = 0; i < 10; i ++) {
		x -> signal[i] = 0;
		x -> signal_handler[i] = NULL;
	}
	
	for (int i = 0; i < 16; i ++) {
		x -> fds[i] = NULL;
	}
	for (int i = 0; i < 3; i ++) {
		x -> fds[i] = my_malloc(sizeof(file*));
		vfs_open("/dev/uart", O_CREAT, &(x -> fds[i]));
	}

	write_sysreg(tpidr_el1, x);

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
void im_fine() {
	uart_printf ("I'm fine\r\n");
}

void schedule() {
	// uart_printf ("thread_start: %llx\r\n", &thread_start);
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
	// uart_printf ("[DEBUG] Scheduled From %d to %d\r\n", get_current() -> id, thread_start -> id);
	switch_to(get_current(), thread_start);
	// switch_page();
	long PGD = read_sysreg(ttbr0_el1);
	// uart_printf ("id is now %d, PGD is now %llx, cur -> PGD is %llx\r\n", get_current() -> id, PGD, get_current() -> PGD); 
	// after successfully switched, ending of irq handler do irq(1)
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
	uart_printf ("Jumping to %llx with sp %llx\r\n", thread_fn[get_current() -> id], (char*)sp + 16 * 2);
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
	long ttbr1 = read_sysreg(ttbr1_el1);
	uart_printf ("TTBR1: %llx\r\n", ttbr1);
	uart_printf ("my_malloc: %llx, is at %llx\r\n", my_malloc, trans(my_malloc));
	thread* x = my_malloc(sizeof(thread));
	x -> stack_start = my_malloc(stack_size * 4);
	x -> kstack_start = my_malloc(stack_size);
	x -> sp = x -> kstack_start + stack_size - 16; 
	x -> fp = x -> sp;
	x -> lr = thread_func_handler;
	x -> state = 1;
	
	x -> PGD = va2pa(my_malloc(4096));
	memset(pa2va(x -> PGD), 0, 4096);
	
	for (int i = 0; i < 10; i ++) {
		x -> signal_handler[i] = NULL;
		x -> signal[i] = 0;
	}

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

	for (int i = 0; i < 16; i ++) {
		x -> fds[i] = NULL;
	}
	/*
	for (int i = 0; i < 3; i ++) {
		x -> fds[i] = my_malloc(sizeof(file*));
		vfs_open("/dev/uart", O_CREAT, &(x -> fds[i]));
	}
	*/


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
	irq(0);
	uart_printf ("started thread testing\r\n");
	thread_create(idle);
	for (int i = 0; i < 3; i ++) {
		thread_create(foo);
	}
	irq(1);
	// schedule();	
}
