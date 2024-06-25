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

list_head_t                 *run_queue;
thread_t                    *threads[MAX_PID + 1];
thread_t                    *curr_thread;
int                         pid_history = 0;
int                         need_to_schedule = 0;
extern char                 _start;
extern char                 _end;

void init_thread_sched() {
    run_queue = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);

    /* idle process */
    char *thread_name = kmalloc(5);
	strcpy(thread_name, "idle");
    curr_thread = _init_thread_0(thread_name, 0, 0, idle);
    set_current(&(threads[0]->context));

    /* init process */
	thread_name = kmalloc(5);
	strcpy(thread_name, "init");
	thread_create(init, thread_name);

    /* shell process */
    thread_name = kmalloc(7);
	sprintf(thread_name, "shell");
	thread_create(cli_start_shell, thread_name);

    schedule_timer();
}

thread_t *_init_thread_0(char* name, int64_t pid, int64_t ppid, void *start) {
    thread_t *thread = (thread_t *)kmalloc(sizeof(thread_t));
    init_thread_signal(thread);
    threads[0] = thread;
    thread->name = name;
	thread->pid = pid;
    thread->ppid = ppid;
	thread->child_list = (child_node_t *)kmalloc(sizeof(child_node_t));
	INIT_LIST_HEAD((list_head_t *)thread->child_list);
    thread->status = THREAD_READY;
    thread->user_stack_base = kmalloc(USTACK_SIZE);
	thread->kernel_stack_base = kmalloc(KSTACK_SIZE);
	thread->context.lr = (uint64_t)start;
	thread->context.sp = (uint64_t)thread->kernel_stack_base + KSTACK_SIZE;
	thread->context.fp = thread->context.sp; // frame pointer for local variable, which is also in stack.
	list_add((list_head_t *)thread, run_queue);
    // dump_thread_info(thread);
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
    init_thread_signal(t);
    threads[new_pid] = t;
    t->name = name;
    t->pid = new_pid;
    t->ppid = curr_thread->pid;
    t->child_list = (child_node_t *)kmalloc(sizeof(child_node_t));
    INIT_LIST_HEAD((list_head_t *)t->child_list);
    t->user_stack_base = kmalloc(USTACK_SIZE);
    t->kernel_stack_base = kmalloc(KSTACK_SIZE);
    t->code = start;
    t->context.lr = (uint64_t)start;
    t->context.sp = (uint64_t)t->kernel_stack_base + KSTACK_SIZE;
    t->context.fp = t->context.sp; // frame pointer for local variable, which is also in stack.

    /* init child node of this thread and add it to the curr_thread's child list */
    child_node_t *child = (child_node_t *)kmalloc(sizeof(child_node_t));
	child->pid = new_pid;
	list_add_tail((list_head_t *)child, (list_head_t *)curr_thread->child_list);

    // dump_thread_info(t);
    list_add((list_head_t *)t, run_queue);
    unlock();
    return t;
}

void thread_exit() {
	// thread cannot deallocate the stack while still using it, wait for someone to recycle it.
	// In this lab, idle thread handles this task, instead of parent thread.
	lock();
	// uart_puts("[-] Thread %d exit\n", curr_thread->pid);
	curr_thread->status = THREAD_ZOMBIE;
	list_del_entry((list_head_t *)curr_thread); // remove from run queue, still in parent's child list
	unlock();
	schedule();
}

void thread_exit_by_pid(int64_t pid) {
	lock();
	// uart_puts("[-] Thread %d exit\n", curr_thread->pid);
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
    unlock();
    // uart_puts("[*] Thread idle(0) is running.\n"); // debug
    // print_lock();
    while (1) {
        schedule();     // switch to next thread in run queue
    }
}

void init() {
    // uart_puts("[*] Thread init(1) is running.\n"); // debug
    // print_lock();
    while (1) {
		int64_t pid = wait();
		uart_puts("[i] exit: %d\n", pid);
	}
}

int64_t wait() {
    lock();
    // uart_puts("[*] Thread %s(%d) is blocked.\n", curr_thread->name, curr_thread->pid); // debug
    curr_thread->status = THREAD_BLOCKED;
    while(1) {
        unlock();
        schedule();
        lock();

		struct list_head *curr_child_node;
        list_head_t *tmp;
        list_for_each_safe(curr_child_node, tmp, (list_head_t *)curr_thread->child_list) {
            thread_t *child_thread = threads[((child_node_t*)curr_child_node)->pid];
            if (child_thread->status == THREAD_ZOMBIE) {
                int64_t pid = child_thread->pid;
                uart_puts("   waiting thread kfree child: %d\n", pid);
                free_child_thread(child_thread);
                list_del_entry(curr_child_node);
                unlock();
                return pid;   
            }
        }
    }
    unlock();
    return 0;
}

void free_child_thread(thread_t *child_thread) {
    list_head_t *curr;
	list_head_t *n;
    uart_puts(YEL "[-] Free child thread PID(%d)\n" CRESET, child_thread->pid);
    list_for_each_safe(curr, n, (list_head_t *)child_thread->child_list) {
		thread_t *curr_child = threads[((child_node_t *)curr)->pid];
		/* assign to init process */
        curr_child->ppid = 1;
		uart_puts("    move child thread: %d to init process\n", curr_child->pid);
		list_del_entry(curr);
		list_add_tail(curr, (list_head_t *)threads[1]->child_list);
	}
    /* check if the code is in kernel space */
    if (thread_code_can_free(child_thread)) {
        uart_puts("    child thread addr: 0x%x, _start: 0x%x, _end: 0x%x\n", child_thread, &_start, &_end);
		kfree(child_thread->code);
	}
    threads[child_thread->pid] = NULL;
	kfree(child_thread->child_list);
	kfree(child_thread->user_stack_base);
	kfree(child_thread->kernel_stack_base);
	kfree(child_thread->name);
    kfree(child_thread);
    uart_puts("    child_thread: 0x%x, curr_thread: 0x%x\n", child_thread, curr_thread);
}


void schedule() {
    lock();
	do {
		curr_thread = (thread_t *)(((list_head_t *)curr_thread)->next);
		// uart_puts("%d: %s -> %d: %s\n", prev_thread->pid, prev_thread->name, curr_thread->pid, curr_thread->name);
	} while (list_is_head((list_head_t *)curr_thread, run_queue)); // find a runnable thread
	curr_thread->status = THREAD_RUNNING;
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

void dump_child_thread(thread_t *thread) {
	list_head_t *curr;
	uart_puts(WHT "[i] Child List        %s(%d)\r\n" CRESET, thread->name, thread->pid);
	list_for_each(curr, (list_head_t *)thread->child_list) {
		list_head_t *curr_child = (list_head_t *)threads[((child_node_t *)curr)->pid];

		uart_puts(WHT "    %s(%d)          0x%x\n" CRESET, ((thread_t *)curr_child)->name, ((thread_t *)curr_child)->pid, curr_child);
        uart_puts(WHT "    ppid              0x%x\n" CRESET, ((thread_t *)curr_child)->ppid);
        uart_puts(WHT "    next              0x%x\n" CRESET, curr_child->next);
        uart_puts(WHT "    prev              0x%x\n" CRESET, curr_child->prev);
        
		switch (((thread_t *)curr_child)->status) {
		case THREAD_RUNNING:
			uart_puts(WHT "    status            " GRN "THREAD_RUNNING\r\n" CRESET);
			break;
		case THREAD_READY:
			uart_puts(WHT "    status            " BLU "THREAD_READY\r\n" CRESET);
			break;
		case THREAD_BLOCKED:
			uart_puts(WHT "    status            " RED "THREAD_BLOCKED\r\n" CRESET);
			break;
		case THREAD_ZOMBIE:  
			uart_puts(WHT "    status            " YEL "THREAD_ZOMBIE\r\n" CRESET);
			break;
		}
	}
    uart_puts("\r\n");
}

void dump_run_queue(thread_t *root, int64_t level) {
	for (int i = 0; i < level; i++)
		uart_puts("   ");
	uart_puts(" |---");
	// dump_thread_info(root);
	list_head_t *curr;
	list_for_each(curr, (list_head_t *)root->child_list) {
		// INFO("child: %d\n", child_node_to_thread((child_node_t *)curr)->pid);
		dump_run_queue(threads[((child_node_t *)curr)->pid], level + 1);
		// ERROR("OVER");
	}
}

int8_t thread_code_can_free(thread_t *thread) {
	return !in_kernel_img_space((uint64_t)thread->code);
}

int8_t in_kernel_img_space(uint64_t addr) {
	// uart_puts("addr: 0x%x, _start: 0x%x, _end: 0x%x\n", addr, &_start, &_end);
	return addr >= &_start && addr < &_end;
}

void foo() {
    // Lab5 Basic 1 Test function
    for (int i = 0; i < 3; ++i) {
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