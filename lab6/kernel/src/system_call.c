#include "system_call.h"
#include "alloc.h"
#include "thread.h"
#include "mail.h"
#include "cpio.h"
#include "mini_uart.h"
#include "helper.h"
#include "exception.h"
#include "utils.h"
#include "mmu.h"

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
	switch_page();

	uart_printf ("Started do fork\r\n");
	int id = thread_create(NULL); // later set by thread_fn

	thread* cur = get_current();
	thread* child = threads[id];

		
	for (int i = 0; i < 10; i ++) {
		child -> reg[i] = cur -> reg[i];
	}
		
	for (int i = 0; i < stack_size; i ++) {
		((char*) child -> kstack_start)[i] = ((char*)(cur -> kstack_start))[i];
	}
	// uart_printf ("finish copying kernel stack\r\n");
	for (int i = 0; i < stack_size * 4; i ++) {
		((char*) child -> stack_start)[i] = ((char*)(cur -> stack_start))[i];
	}
	// uart_printf ("finish copying user stack\r\n");

	int t = (cur -> code_size + 4096 - 1) / 4096 * 4096;
	child -> code = my_malloc(t);
	child -> code_size = cur -> code_size;
	
	for (int i = 0; i < t; i ++) {
		((char*)child -> code)[i] = ((char*)(cur -> code))[i];
	}
	// uart_printf ("finish copying code\r\n");

	setup_peripheral_identity(pa2va(child -> PGD));
	// uart_printf ("finish mapping peripheral\r\n");
	for (int i = 0; i < t / 4096; i ++) {
		map_page(pa2va(child -> PGD), i * 4096, va2pa(child -> code) + i * 4096, (1 << 6));
	}
	// uart_printf ("finish mapping code\r\n");
	for (int i = 0; i < 4; i ++) {
		map_page(pa2va(child -> PGD), 0xffffffffb000L + i * 4096, va2pa(child -> stack_start) + i * 4096, (1 << 6));
	}
	// uart_printf ("finish mapping stack\r\n");

	child -> stack_start = 0xffffffffb000;
	child -> code = 0;
	
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
	uart_printf ("CUR SPEL0: %llx, CUR SPEL1: %llx\r\n", sp_el0, sp_el1);

	trapframe_t* child_tf = (trapframe_t*)child -> sp;
	child_tf -> x[0] = 0;
	child_tf -> sp_el0 += child -> stack_start - cur -> stack_start;

	//elr_el1 should be the same 
	
	uart_printf ("Child tf should be at %llx should jump to %llx\r\n", child -> sp, thread_fn[id], child_tf -> elr_el1);
	uart_printf ("And then to %llx, ori is %llx :(:(:(:(:\r\n", child_tf -> elr_el1, tf -> elr_el1);

	// uart_printf ("diff1 %x, diff2 %x\r\n", (void*)tf - (cur -> kstack_start), (void*)child_tf - (child -> kstack_start));
	
	return id;	
}

void do_exit() {
	kill_thread(get_current() -> id);
}

int do_mbox_call(unsigned char ch, unsigned int *mbox) {
	int* t = my_malloc(4096);
	for (int i = 0; i < 144; i ++) {
		((char*)t)[i] = ((char*)mbox)[i];
	}
	int res = mailbox_call(t, ch);
	for (int i = 0; i < 144; i ++) {
		((char*)mbox)[i] = ((char*)t)[i];
	}
	return res;
	
	// return mailbox_call(mbox, ch);
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

extern void uart_write(char);
void simple_fork_test() {
	uart_write('f');
	while (1);
	// uart_printf ("In simple fork test\r\n");

	/*
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
	*/
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

extern char _code_start[];

void im_fineee() {
	long elr_el1 = read_sysreg(elr_el1);
	long sp_el0 = read_sysreg(sp_el0);

	uart_printf ("im fine, elr_el1 %llx, sp_el0 %llx\r\n", elr_el1, sp_el0);
}

void from_el1_to_fork_test() {
	irq(0);
	// switch_page();
	uart_printf ("%llx\r\n", get_current() -> PGD);
	long ttbr0 = read_sysreg(ttbr0_el1);
	long ttbr1 = read_sysreg(ttbr1_el1);
	uart_printf ("ttbr0: %llx, ttbr1: %llx\r\n", ttbr0, ttbr1);
	void* sp = 0xfffffffff000 - 16;
	void* code = (char*)simple_fork_test - _code_start;

	long test = code;
	uart_printf ("%llx translated to %llx\r\n", test, trans(test));
	uart_printf ("%llx translated to %llx\r\n", test, trans_el0(test));

	uart_printf ("stack: %llx code: %llx\r\n", sp, code);

	/*
	for (int i = 0; i < 100; i ++) {
		uart_printf ("%d ", ((char*)code)[i]);
	}
	uart_printf ("\r\n");
	*/
	
	/*
	for (int i = 0; i < 100; i ++) {
		uart_printf ("%d ", ((char*)simple_fork_test)[i]);
	}
	uart_printf ("\r\n");
	*/

	// simple_fork_test();

	/*
	void (*func)() = code;
	func();
	*/
	
	asm volatile(
		"mov x1, 0;"
		"msr spsr_el1, x1;"
		"mov x1, %[code];"
		"mov x2, %[stack];"
		"msr elr_el1, x1;"
		"msr sp_el0, x2;"
		// "msr DAIFclr, 0xf;" // irq(1);
		"eret;"
		:
		: [code] "r" (code), [stack] "r" (sp)
		: "x1", "x2"
	);

	uart_printf ("fuck you\r\n");
	asm volatile ( "eret;" );
}
