#include "sched.h"
#include "memory.h"
#include "timer.h"
#include "list.h"
#include "stddef.h"
#include "exception.h"
#include "callback_adapter.h"
#include "uart1.h"

struct list_head *run_queue;

static int64_t pid_history = 0;
int8_t need_to_schedule = 0;

static inline thread_t *child_node_to_thread(child_node_t *node)
{
	return threads[(node)->pid];
}

static inline int free_thread(int64_t pid)
{
	if(pid >= MAX_PID){
		return -1;
	}
	thread_t *thread = threads[pid];
	kfree(thread->user_stack_base);
	kfree(thread->kernel_stack_base);
	kfree(thread);
	threads[pid] = NULL;
	return 0;
}

void init_thread_sched()
{
	lock_interrupt();
	run_queue = kmalloc(sizeof(thread_t));
	INIT_LIST_HEAD(run_queue);

	char *thread_name = kmalloc(5);
	strcpy(thread_name, "idle");
	_init_create_thread(thread_name, 0, 0, idle);
	thread_name = kmalloc(5);
	strcpy(thread_name, "init");
	curr_thread = thread_create(init, thread_name);
	unlock_interrupt();
}

void _init_create_thread(char *name, int64_t pid, int64_t ppid, void *start)
{
	thread_t *thread = (thread_t *)kmalloc(sizeof(thread_t));
	curr_thread = thread;
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
	thread->context.sp = (uint64_t)thread->user_stack_base + USTACK_SIZE;
	thread->context.fp = thread->context.sp; // frame pointer for local variable, which is also in stack.
	list_add((list_head_t *)thread, run_queue);
}

void idle()
{
	while (1)
	{
		// wait();
		schedule();
	}
}

void init()
{
	while (1)
	{
		int c_pid = wait();
		DEBUG("child process %d exit\n", c_pid);
	}
}

int64_t wait()
{
	lock_interrupt();
	DEBUG("block thread: %d\n", curr_thread->pid);
	curr_thread->status = THREAD_BLOCKED;
	while (1)
	{
		unlock_interrupt();
		schedule();
		lock_interrupt();
		DEBUG({
			dump_chile_thread(curr_thread);
		});
		struct list_head *curr_child_node;
		list_head_t *n;
		list_for_each_safe(curr_child_node, n, (list_head_t *)curr_thread->child_list)
		{
			thread_t *child_thread = child_node_to_thread((child_node_t *)curr_child_node);
			if (child_thread->status == THREAD_ZOMBIE)
			{
				int64_t pid = child_thread->pid;
				DEBUG("wait thread kfree\n");
				free_thread(pid);
				list_del_entry(curr_child_node);
				unlock_interrupt();
				return pid;
			}
		}
	}
	unlock_interrupt();
	return 0;
}

void thread_exit()
{
	// thread cannot deallocate the stack while still using it, wait for someone to recycle it.
	// In this lab, idle thread handles this task, instead of parent thread.
	lock_interrupt();
	DEBUG("thread %d exit\n", curr_thread->pid);
	curr_thread->status = THREAD_ZOMBIE;
	thread_t *exit_thread = (list_head_t *)curr_thread;
	do
	{
		curr_thread = (thread_t *)(((list_head_t *)curr_thread)->next);
	} while (list_is_head((list_head_t *)curr_thread, run_queue) || (curr_thread->status == THREAD_ZOMBIE)); // find a runnable thread
	list_del_entry((list_head_t *)exit_thread);
	list_head_t *curr;
	list_head_t *n;
	list_for_each_safe(curr, n, (list_head_t *)exit_thread->child_list)
	{
		thread_t *curr_child = child_node_to_thread((child_node_t *)curr);
		curr_child->ppid = 1;
		list_del_entry(curr);
		list_add_tail(curr, (list_head_t *)threads[1]->child_list);
	}
	DEBUG({
		dump_run_queue();
	});
	unlock_interrupt();
	schedule();
}

void schedule_timer()
{
	uint64_t cntfrq_el0;
	DEBUG("schedule timer\n");
	__asm__ __volatile__("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq_el0));
	// 32 * default timer -> trigger next schedule timer
	add_timer_by_tick(cntfrq_el0 >> 5, adapter_schedule_timer, NULL);
	need_to_schedule = 1;
}

thread_t *thread_create(void *start, char *name)
{
	lock_interrupt();
	thread_t *r;
	int64_t new_pid = -1;
	// find usable PID, don't use the previous one
	for (int i = 1; i < MAX_PID; i++)
	{
		if (threads[pid_history + i] == NULL)
		{
			new_pid = pid_history + i;
			break;
		}
	}
	if (new_pid == -1)
	{ // no available pid
		unlock_interrupt();
		return NULL;
	}
	else
	{
		pid_history = new_pid;
	}
	r = (thread_t *)kmalloc(sizeof(thread_t));
	DEBUG("new_pid: %d, thread address: 0x%x\n", new_pid, r);
	threads[new_pid] = r;
	r->name = name;
	r->pid = new_pid;
	r->ppid = curr_thread->pid;
	r->child_list = (child_node_t *)kmalloc(sizeof(child_node_t));
	INIT_LIST_HEAD((list_head_t *)r->child_list);
	r->status = THREAD_READY;
	r->user_stack_base = kmalloc(USTACK_SIZE);
	r->kernel_stack_base = kmalloc(KSTACK_SIZE);
	r->context.lr = (uint64_t)start;
	r->context.sp = (uint64_t)r->user_stack_base + USTACK_SIZE;
	r->context.fp = r->context.sp; // frame pointer for local variable, which is also in stack.

	child_node_t *child = (child_node_t *)kmalloc(sizeof(child_node_t));
	child->pid = new_pid;
	list_add_tail((list_head_t *)child, (list_head_t *)curr_thread->child_list);

	DEBUG("add new thread: %d, run_que: 0x%x\n", r->pid, run_queue);
	list_add((list_head_t *)r, run_queue);
	DEBUG({
		dump_run_queue();
	});

	DEBUG("add new thread: %d\n", r->pid);
	unlock_interrupt();
	return r;
}

void schedule()
{
	lock_interrupt();
	thread_t *prev_thread = curr_thread;
	do
	{
		curr_thread = (thread_t *)(((list_head_t *)curr_thread)->next);
	} while (list_is_head((list_head_t *)curr_thread, run_queue) || (curr_thread->status == THREAD_ZOMBIE)); // find a runnable thread
	curr_thread->status = THREAD_RUNNING;
	DEBUG("%d switch to %d\n", prev_thread->pid, curr_thread->pid);
	DEBUG({
		dump_run_queue();
	});
	unlock_interrupt();
	switch_to(get_current_thread_context(), &(curr_thread->context));
}

void foo()
{
	// Lab5 Basic 1 Test function
	for (int i = 0; i < 10; ++i)
	{
		uart_puts("Thread id: %d %d\n", curr_thread->pid, i);
#ifdef QEMU
		for (int i = 0; i < 10000000; i++) // qemu
#else
		for (int i = 0; i < 100000; i++) // pi
#endif
		{
			asm volatile("nop");
		}
		// schedule();
	}
	INFO("%s exit\n", curr_thread->name);
	thread_exit();
}

void dump_run_queue()
{
	struct list_head *curr;
	uart_puts("---------------------- run queue list for each ----------------------\r\n");
	list_for_each(curr, (list_head_t *)run_queue)
	{
		uart_puts("thread: 0x%x, thread->next: 0x%x, thread->prev: 0x%x, thread->name: %s, thread->pid: %d, thread->ppid: %d", curr, curr->next, curr->prev, ((thread_t *)curr)->name, ((thread_t *)curr)->pid, ((thread_t *)curr)->ppid);
		switch (((thread_t *)curr)->status)
		{
		case THREAD_RUNNING:
			uart_puts(", thread->status: THREAD_RUNNING\r\n");
			break;
		case THREAD_READY:
			uart_puts(", thread->status: THREAD_READY\r\n");
			break;
		case THREAD_BLOCKED:
			uart_puts(", thread->status: THREAD_BLOCKED\r\n");
			break;
		case THREAD_ZOMBIE:
			uart_puts(", thread->status: THREAD_ZOMBIE\r\n");
			break;
		}
	}
	uart_puts("--------------------------------- end -------------------------------\r\n");
}

void dump_chile_thread(thread_t *thread)
{
	list_head_t *curr;
	uart_puts("---------------------- child list for each ----------------------\r\n");
	list_for_each(curr, (list_head_t *)thread->child_list)
	{
		list_head_t *curr_child = (list_head_t *)threads[((child_node_t *)curr)->pid];

		uart_puts("thread: 0x%x, thread->next: 0x%x, thread->prev: 0x%x, thread->name: %s, thread->pid: %d, thread->ppid: %d", curr_child, curr_child->next, curr_child->prev, ((thread_t *)curr_child)->name, ((thread_t *)curr_child)->pid, ((thread_t *)curr_child)->ppid);
		switch (((thread_t *)curr_child)->status)
		{
		case THREAD_RUNNING:
			uart_puts(", thread->status: THREAD_RUNNING\r\n");
			break;
		case THREAD_READY:
			uart_puts(", thread->status: THREAD_READY\r\n");
			break;
		case THREAD_BLOCKED:
			uart_puts(", thread->status: THREAD_BLOCKED\r\n");
			break;
		case THREAD_ZOMBIE:
			uart_puts(", thread->status: THREAD_ZOMBIE\r\n");
			break;
		}
	}
	uart_puts("----------------------------- end ------------------------------\r\n");
}