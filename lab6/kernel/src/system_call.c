#include "system_call.h"
#include "alloc.h"
#include "thread.h"
#include "mail.h"
#include "cpio.h"
#include "mini_uart.h"
#include "helper.h"
#include "exception.h"

extern thread* get_current();
extern thread** threads;
extern void** thread_fn;

#define stack_size 4096

int do_getpid() {
	return get_current() -> id;
}

size_t do_uart_read(char buf[], size_t size) {
	buf[0] = uart_recv();
	return 1;
}

size_t do_uart_write(char buf[], size_t size) {
	uart_send (buf[0]);
	return 1;
}

int do_exec(char* name, char *argv[]) {
	cpio_load(name);
}

extern void ret_from_fork_child();

typedef unsigned long long ull;

uint64_t get_sp_el0() {
    uint64_t value;
    asm volatile("mrs %0, sp_el0" : "=r" (value));
    return value;
}
uint64_t get_sp_el1() {
    uint64_t value;
    asm volatile("mov %0, sp" : "=r" (value));
    return value;
}



// tf is the sp
int do_fork(trapframe_t* tf) {
	uart_printf ("Started do fork\r\n");
	int id = thread_create(NULL); // later set by thread_fn

	thread* cur = get_current();
	thread* child = threads[id];
		
	for (int i = 0; i < 10; i ++) {
		child -> reg[i] = cur -> reg[i];
	}
	strcpy(cur -> stack_start, child -> stack_start, stack_size);
	strcpy(cur -> kstack_start, child -> kstack_start, stack_size);
	
	child -> sp = (void*)((char*)tf - (char*)cur -> kstack_start + (char*)child -> kstack_start); 
	child -> fp = child -> sp; 
	// on el1 stack
	thread_fn[id] = ret_from_fork_child; 
	
	for (int i = 0; i < 10; i ++) {
		child -> signal_handler[i] = cur -> signal_handler[i];
	}

	if (get_current() -> id == id) {
		uart_printf ("Child should not be here%d, %d\r\n", get_current() -> id, id);
	}

	void* sp_el0 = get_sp_el0();
	void* sp_el1 = get_sp_el1();
	uart_printf ("CUR SPEL0: %x, CUR SPEL1: %x\r\n", sp_el0, sp_el1);

	trapframe_t* child_tf = (trapframe_t*)child -> sp;
	child_tf -> x[0] = 0;
	child_tf -> sp_el0 += child -> stack_start - cur -> stack_start;
	
	uart_printf ("Child tf should be at %x should jump to %x\r\n", child -> sp, thread_fn[id], child_tf -> elr_el1);
	uart_printf ("And then to %x, ori is %x :(:(:(:(:\r\n", child_tf -> elr_el1, tf -> elr_el1);

	uart_printf ("diff1 %x, diff2 %x\r\n", (void*)tf - (cur -> kstack_start), (void*)child_tf - (child -> kstack_start));
	
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

void do_signal(int sig, handler func) {
	uart_printf ("Registered %d\r\n", sig);
	if (sig >= 10) {
		uart_printf ("this sig num is greater then I can handle\r\n");
		return;
	}
	get_current() -> signal_handler[sig] = (void*)func;
}

void do_sigkill(int pid, int sig) {
	uart_printf ("DO sigkill %d, %d\r\n", pid, sig);
	if (sig >= 10) {
		uart_printf ("this sig num is greater then I can handle\r\n");
		return;
	}
	threads[pid] -> signal[sig] = 1;
}

extern int fork();
extern int getpid();
extern void exit();
extern void core_timer_enable();
extern int mbox_call(unsigned char, unsigned int*);
extern void test_syscall();

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
	while (1) {
		uart_printf ("This is main create fork shit\r\n");
		delay(1e7);
	}
}

void fork_test_func() {
	for (int i = 0; i < 3; i ++) {
		int ret = fork();
		if (ret) {
			uart_printf ("This is father %d with child of pid %d\r\n", getpid(), ret);
		}
		else {
			uart_printf ("This is child forked, my pid is %d\r\n", getpid());
		}
	}
	exit();
	/*
	while (1) {
		delay(1e7);
	}
	*/
}
void simple_fork_test() {
	for (int i = 0; i < 1; i ++) {
		// uart_printf ("%d: %d\r\n", getpid(), i);
		int t = fork();
		if (t) {
			uart_printf ("This is parent with child %d\r\n", t);
		}
		else {
			uart_printf ("This is child with pid %d\r\n", getpid());
		}
	}
	exit();
	while (1);
}
void fork_test_idle() {
	while(1) {
		unsigned int __attribute__((aligned(16))) mailbox[7];
        mailbox[0] = 7 * 4; // buffer size in bytes
        mailbox[1] = REQUEST_CODE;
        // tags begin
        mailbox[2] = GET_BOARD_REVISION; // tag identifier
        mailbox[3] = 4; // maximum of request and response value buffer's length.
        mailbox[4] = TAG_REQUEST_CODE;
        mailbox[5] = 0; // value buffer
        // tags end
        mailbox[6] = END_TAG;

        if (mbox_call(8, mailbox)) {
			uart_printf ("%x\r\n", mailbox[5]);
        }
		else {
			uart_printf ("Fail getting mailbox\r\n");
		}

		uart_printf ("This is idle, pid %d, %d\r\n", getpid(), threads[1] -> next -> id);
		delay(1e7);
	}
}

void from_el1_to_fork_test() {
	irq(0);
	uart_printf ("From el1 to el0 fork test\r\n");	
	get_current() -> stack_start = my_malloc(4096);
	void* stack = get_current() -> stack_start + 4096 - 16;

	asm volatile(
		"mov x1, 0;"
		"msr spsr_el1, x1;"
		"mov x1, %[code];"
		"mov x2, %[stack];"
		"msr elr_el1, x1;"
		"msr sp_el0, x2;"
		"msr DAIFclr, 0xf;" // irq(1);
		"eret;"
		:
		: [code] "r" (simple_fork_test), [stack] "r" (stack)
		: "x1", "x30"
	);
}
