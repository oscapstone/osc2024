#include "system_call.h"
#include "alloc.h"
#include "thread.h"
#include "mail.h"
#include "cpio.h"
#include "mini_uart.h"
#include "helper.h"
#include "exception.h"

extern thread* get_current();
extern thread* threads[];
extern void* thread_fn[];

const int stack_size = 4096; 

int do_getpid() {
	return get_current() -> id;
}

size_t do_uart_read(char buf[], size_t size) {
	for (int i = 0; i < size; i ++) {
		buf[i] = uart_recv(); 
	}
	return size;
}

size_t do_uart_write(char buf[], size_t size) {
	for (int i = 0; i < size; i ++) {
		uart_send(buf[i]);
	}
	return size;
}

int do_exec(char* name, char *argv[]) {
	void* pos = cpio_find(name);
	void* code = my_malloc(4096);
	strcpy(pos, code, 4096);

	void* stack = my_malloc(4096);
	stack += 4096 - 16;

	void (*func)() = pos;
	func();

}


void fork_test_idle();
extern void ret_from_fork_child();

inline void get_sp() {
	trapframe_t* sp;
	asm volatile (
		"mov %0, sp\n"
		: "=r" (sp)
	);
	uart_printf ("sp is %x, elr is %x\r\n", sp, sp -> elr_el1);
	return;
}

void get_elr_el1() {
	unsigned long elr_el1;
	asm volatile (
		"mrs %0, elr_el1\n"
		: "=r" (elr_el1)
	);
	uart_printf ("elr_el1 is %x\r\n", elr_el1);
	return;
}

void c_ret_from_fork_child() {
	unsigned long el;
	asm volatile (
		"mrs %0, CurrentEL\n"
		"lsr %0, %0, #2\n"
		: "=r" (el)
	);
	uart_printf ("Going to load_all and eret, el %d\r\n", el);
	
	trapframe_t* sp;
	asm volatile (
		"mov %0, sp\n"
		: "=r" (sp)
	);
	uart_printf ("sp is %x, elr is %x\r\n", sp, sp -> elr_el1);

	asm volatile ( "bl ret_from_fork_child" );
}

typedef unsigned long long ull;

// tf is the sp
int do_fork(trapframe_t* tf) {

	irq (0);
	uart_printf ("Started do fork\r\n");
	int id = thread_create(NULL); // later set by thread_fn

	thread* cur = get_current();
	thread* child = threads[id];
		
	for (int i = 0; i < 10; i ++) {
		child -> reg[i] = cur -> reg[i];
	}
	strcpy(cur -> stack_start, child -> stack_start, stack_size);
	
	child -> sp = ((ull)tf - (ull)cur -> stack_start + (ull)child -> stack_start); 
	child -> fp = child -> sp; 
	thread_fn[id] = ret_from_fork_child; 

	if (get_current() -> id == id) {
		uart_printf ("Child should not be here%d, %d\r\n", get_current() -> id, id);
	}

	trapframe_t* child_tf = (trapframe_t*)child -> sp;
	child_tf -> x[0] = 0;
	uart_printf ("Forked a child with pid = %d\r\n", id);
	
	irq(1);
	
	return id;
}

void do_exit() {
	kill_thread(get_current() -> id);
}

int do_mbox_call(unsigned char ch, unsigned int *mbox) {
	return mailbox_call(mbox, ch);
}

void do_kill(int pid) {
	kill_thread(pid);
}

extern int fork();
extern int getpid();
extern void exit();
extern void core_timer_enable();

void given_fork_test(){
    uart_printf("\nFork Test, pid %d\n", getpid());
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        uart_printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        ++cnt;

        if ((ret = fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            uart_printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                uart_printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
                delay(1000000);
                ++cnt;
            }
        }
        exit();
    }
    else {
        uart_printf("parent here, pid %d, child %d\n", getpid(), ret);
    }
}

void fork_test_func() {
	int ret = fork();
	if (ret) {
		uart_printf ("This is father with child of pid %d\r\n", ret);
	}
	else {
		uart_printf ("This is child forked, my pid is %d\r\n", getpid());
	}
	while (1) {
		uart_printf ("This is fork test func, pid %d\r\n", getpid(), ret);
		delay(1e7);
	}
}
void fork_test_idle() {
	while(1) {
		unsigned int el;
		uart_printf ("This is idle, pid %d, %d\r\n", getpid(), threads[1] -> next -> id);
		delay(1e7);
	}
}

void fork_test() {
	uart_printf ("started fork test\r\n");
	thread_create(fork_test_idle);
	thread_create(given_fork_test);
	uart_printf ("Finish creating functions\r\n");
	unsigned int el;
	/*
	asm volatile (
		"mrs %0, CurrentEL\n"
		"lsr %0, %0, #2\n"
		: "=r" (el)
	);
	*/
	while (1) {
		uart_printf ("Original fork test place, %d\r\n", threads[1] -> next -> id);
		delay(1e7);
	}
}
	
void from_el1_to_fork_test() {
	uart_printf ("From el1 to fork test\r\n");	
	void* stack = my_malloc(4096);
	stack += 4096 - 16;
	asm volatile(
		"mov x1, 0;"
		"msr spsr_el1, x1;"
		"mov x1, %[code];"
		"mov x2, %[sp];"
		"msr elr_el1, x1;"
		"msr sp_el0, x2;"
		"eret;"
		:
		: [code] "r" (fork_test), [sp] "r" (stack)
		: "x1", "x30"
	);
}
