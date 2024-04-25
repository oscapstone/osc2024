#include "timer.h"
#include "list.h"
#include "stddef.h"
#include "sched.h"
#include "memory.h"
#include "exception.h"
#include "mini_uart.h"
#include "shell.h"
#include "utils.h"
#include "ANSI.h"

list_head_t     *run_queue;
thread_t        *threads[MAX_PID + 1];
thread_t        *curr_thread;
int             pid_history = 0;
int             need_to_schedule = 0;

void init_thread_sched() {
    run_queue = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);

    char *thread_name = kmalloc(5);
	strcpy(thread_name, "idle");
    curr_thread = _init_thread_0(thread_name, 0, idle);
    set_current(&(threads[0]->context));

    thread_name = kmalloc(7);
	sprintf(thread_name, "shell");
	thread_create(cli_start_shell, thread_name);

    schedule_timer();
}

thread_t *_init_thread_0(char* name, int64_t pid, void *start) {
    thread_t *thread = (thread_t *)kmalloc(sizeof(thread_t));
    threads[0] = thread;
    thread->name = name;
	thread->pid = pid;
    thread->status = THREAD_READY;
    thread->user_stack_base = kmalloc(USTACK_SIZE);
	thread->kernel_stack_base = kmalloc(KSTACK_SIZE);
	thread->context.lr = (uint64_t)start;
	thread->context.sp = (uint64_t)thread->kernel_stack_base + KSTACK_SIZE;
	thread->context.fp = thread->context.sp; // frame pointer for local variable, which is also in stack.
	list_add((list_head_t *)thread, run_queue);
    dump_thread_info(thread);
	return thread;
}

thread_t *thread_create(void *start, char* name) {
    lock();
    thread_t *t;
    int new_pid = -1;
    
    /* Get new PID */
    for (int i = 1; i < MAX_PID; i++ ) {
        if (threads[pid_history + i] == NULL) {
            new_pid = pid_history + i;
            break;
        }
    }
    /* Check and update PID history */
    if (new_pid == -1) {
        uart_puts("[!] No available PID. Fork failed.\n");
        unlock();
        return NULL;
    } else {
        pid_history = new_pid;
    }

    t = (thread_t*)kmalloc(sizeof(thread_t));
    threads[new_pid] = t;
    t->name = name;
    t->pid = new_pid;
    t->user_stack_base = kmalloc(USTACK_SIZE);
    t->kernel_stack_base = kmalloc(KSTACK_SIZE);
    t->code = start;
    t->context.lr = (uint64_t)start;
    t->context.sp = (uint64_t)t->kernel_stack_base + KSTACK_SIZE;
    t->context.fp = t->context.sp; // frame pointer for local variable, which is also in stack.

    dump_thread_info(t);
    list_add_tail(&t->listhead, run_queue);
    unlock();
    return t;
}

void thread_exit() {
	// thread cannot deallocate the stack while still using it, wait for someone to recycle it.
	// In this lab, idle thread handles this task, instead of parent thread.
	lock();
	uart_puts("[-] Thread %d exit\n", curr_thread->pid);
	curr_thread->status = THREAD_ZOMBIE;
	list_del_entry((list_head_t *)curr_thread); // remove from run queue, still in parent's child list
	unlock();
	schedule();
}

void thread_exit_by_pid(int64_t pid) {
	lock();
	uart_puts("[-] Thread %d exit\n", curr_thread->pid);
    thread_t *t = threads[pid];
	t->status = THREAD_ZOMBIE;
	list_del_entry((list_head_t *)t); 
	unlock();
	schedule();
}

void dump_thread_info(thread_t* t) {
    uart_puts(YEL "[+] New thread(PID)   %s(%d)\n" CRESET, t->name, t->pid);
    uart_puts(GRN "    thread_address    0x%x\n" CRESET, t);
    uart_puts(GRN "    user_stack_base   0x%x\n" CRESET, t->user_stack_base);
    uart_puts(GRN "    kernel_stack_base 0x%x\n" CRESET, t->kernel_stack_base);
    uart_puts(GRN "    context.sp        0x%x\n" CRESET, t->context.sp);
    uart_puts(GRN "    run_queue         0x%x\n" CRESET, t->user_stack_base);
    uart_puts("\r\n");
}

void idle() {
    while (1) {
        // uart_puts("[*] Thread idle(0) is running.\n"); // debug
        schedule();     // switch to next thread in run queue
    }
}

void schedule() {
    lock();
	do {
		curr_thread = (thread_t *)(((list_head_t *)curr_thread)->next);
		// DEBUG("%d: %s -> %d: %s\n", prev_thread->pid, prev_thread->name, curr_thread->pid, curr_thread->name);
	} while (list_is_head((list_head_t *)curr_thread, run_queue)); // find a runnable thread
	curr_thread->status = THREAD_RUNNING;
	// uint64_t ttbr0_el1_value;
	// asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0_el1_value));
	// DEBUG("PGD: 0x%x, ttbr0_el1: 0x%x\r\n", curr_thread->context.pgd, ttbr0_el1_value);
	// DEBUG("curr_thread->pid: %d\r\n", curr_thread->pid);
	unlock();
    // uart_puts("[*] Schedule to %s(%d)\n", curr_thread->name, curr_thread->pid); // debug
    switch_to(get_current(), &(curr_thread->context));
}

void schedule_timer() {
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0));
	// 32 * default timer -> trigger next schedule timer
	add_timer_by_tick(schedule_timer, cntfrq_el0 >> 5, NULL);
	need_to_schedule = 1;
}

void foo() {
    // Lab5 Basic 1 Test function
    for (int i = 0; i < 3; ++i)
    {
        uart_puts("foo() thread pid is %d in for loop index = %d\n", curr_thread->pid, i);
        int r = 1000000;
        while (r--) {
            asm volatile("nop");
        }
        schedule();
        uart_puts("In for loop - foo() thread pid is %d in for loop index = %d\n", curr_thread->pid, i);
    }
    thread_exit();
}

// void schedule_timer() {
//     unsigned long long cntfrq_el0;
//     __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0));
//     add_timer(schedule_timer, cntfrq_el0 >> 5, "", 1);
//     // 32 * default timer -> trigger next schedule timer
// }

// void exec_thread(char *data, unsigned int filesize) {
//     lock();
//     thread_t *t = thread_create(data);
//     t->data = kmalloc(filesize);
//     t->datasize = filesize;
//     t->context.lr = (unsigned long)t->data; // set return address to program if function call completes
//     // copy file into data
//     for (int i = 0; i < filesize;i++)
//     {
//         t->data[i] = data[i];
//     }
//     curr_thread = t;
//     add_timer(schedule_timer, 2, "", 0); // start scheduler
//     // eret to exception level 0
//     asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
//         "msr elr_el1, %1\n\t"   // When el0 -> el1, store return address for el1 -> el0
//         "msr spsr_el1, xzr\n\t" // Enable interrupt in EL0 -> Used for thread scheduler
//         "msr sp_el0, %2\n\t"    // el0 stack pointer for el1 process, user program stack pointer set to new stack.
//         "mov sp, %3\n\t"        // sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
//         ::"r"(&t->context),"r"(t->context.lr), "r"(t->stack_allocated_base + USTACK_SIZE), "r"(t->context.sp));
//     unlock();

//     asm("eret\n\t");
// }

