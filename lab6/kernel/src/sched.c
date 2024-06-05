#include "sched.h"
#include "memory.h"
#include "timer.h"
#include "list.h"
#include "stddef.h"
#include "string.h"
#include "syscall.h"
#include "exception.h"
#include "callback_adapter.h"
#include "uart1.h"
#include "shell.h"

struct list_head *run_queue;

thread_t *threads[MAX_PID + 1];
thread_t *curr_thread;
extern char _kernel_start;
extern char _kernel_end;

static int64_t pid_history = 0;
int8_t need_to_schedule = 0;

static inline int8_t in_kernel_img_space(uint64_t addr)
{
	DEBUG("addr: 0x%x, _start: 0x%x, _end: 0x%x\n", addr, &_kernel_start, &_kernel_end);
	return addr >= &_kernel_start && addr < &_kernel_end;
}

static inline thread_t *child_node_to_thread(child_node_t *node)
{
	return threads[(node)->pid];
}

static inline int free_child_thread(thread_t *child_thread)
{
	list_head_t *curr;
	list_head_t *n;
	DEBUG("free child thread: %d\n", child_thread->pid);

	list_for_each_safe(curr, n, (list_head_t *)child_thread->child_list)
	{
		thread_t *curr_child = child_node_to_thread((child_node_t *)curr);
		curr_child->ppid = 1; // assign to init process
		DEBUG("move child thread: %d to init process\n", curr_child->pid);
		list_del_entry(curr);
		list_add_tail(curr, (list_head_t *)threads[1]->child_list);
	}

	// if (thread_code_can_free(child_thread))
	// {
	// 	kfree(child_thread->code);
	// }
	threads[child_thread->pid] = NULL;
	mmu_free_all_vma(child_thread);
	kfree(child_thread->vma_list);
	kfree(child_thread->child_list);
	// kfree(child_thread->user_stack_bottom);
	kfree(child_thread->kernel_stack_bottom);
	kfree(child_thread->name);
	kfree(child_thread);
	DEBUG("child_thread: 0x%x, curr_thread: 0x%x\n", child_thread, curr_thread);
	// dump_run_queue();
	return 0;
}

inline int8_t thread_code_can_free(thread_t *thread)
{
	return !in_kernel_img_space((uint64_t)thread->code);
}

/**
 * @brief Initialize the run queue and create idle and init process
 *
 * Set the current thread context to idle process
 */
void init_thread_sched()
{
	kernel_lock_interrupt();

	run_queue = kmalloc(sizeof(thread_t));
	INIT_LIST_HEAD(run_queue);

	// idle process
	char *thread_name = kmalloc(5);
	strcpy(thread_name, "idle");
	curr_thread = _init_create_thread(thread_name, 0, 0, idle);
	set_current_thread_context(&(threads[0]->context));

	// init process
	thread_name = kmalloc(5);
	strcpy(thread_name, "init");
	thread_t *init_thread = thread_create(init, thread_name);
	init_thread->datasize = 0x4000;

	// kshell process
	thread_name = kmalloc(7);
	sprintf(thread_name, "kshell");
	thread_t *kshell = thread_create(start_shell, thread_name);
	kshell->datasize = 0x100000;
	schedule_timer();
	DEBUG("init_thread: 0x%x, kshell: 0x%x\n", init_thread, kshell);
	kernel_unlock_interrupt();
}

thread_t *_init_create_thread(char *name, int64_t pid, int64_t ppid, void *start)
{
	thread_t *thread = (thread_t *)kmalloc(sizeof(thread_t));
	init_thread_signal(thread);
	threads[0] = thread;
	thread->name = name;
	thread->pid = pid;
	thread->ppid = ppid;
	thread->child_list = (child_node_t *)kmalloc(sizeof(child_node_t));
	INIT_LIST_HEAD((list_head_t *)thread->child_list);
	thread->vma_list = (vm_area_struct_t *)kmalloc(sizeof(vm_area_struct_t));
	INIT_LIST_HEAD((list_head_t *)thread->vma_list);
	thread->status = THREAD_READY;
	thread->user_stack_bottom = kmalloc(USTACK_SIZE);
	thread->kernel_stack_bottom = kmalloc(KSTACK_SIZE);
	thread->context.lr = (uint64_t)start;
	thread->context.sp = (uint64_t)thread->kernel_stack_bottom + KSTACK_SIZE;
	thread->context.fp = thread->context.sp; // frame pointer for local variable, which is also in stack.
	list_add((list_head_t *)thread, run_queue);
	return thread;
}

void idle()
{
	kernel_unlock_interrupt();
	DEBUG("idle process\r\n");
	while (1)
	{
		schedule();
	}
}

void init()
{
	while (1)
	{
		int64_t pid = wait();
		DEBUG("exit: %d\n", pid);
	}
}

void __init()
{
	DEBUG("user space init process\n");
	while (1)
	{
		DEBUG("wait\n");
		int c_pid = wait();
		DEBUG("child process %d exit\n", c_pid);
	}
}

int64_t wait()
{
	kernel_lock_interrupt();
	DEBUG("block thread: %d\n", curr_thread->pid);
	curr_thread->status = THREAD_BLOCKED;
	while (1)
	{
		// DEBUG("wait thread: %d\n", curr_thread->pid);
		kernel_unlock_interrupt();
		schedule();
		kernel_lock_interrupt();
		// DEBUG_BLOCK({
		// 	dump_child_thread(curr_thread);
		// });
		struct list_head *curr_child_node;
		list_head_t *n;
		list_for_each_safe(curr_child_node, n, (list_head_t *)curr_thread->child_list)
		{
			thread_t *child_thread = child_node_to_thread((child_node_t *)curr_child_node);
			if (child_thread->status == THREAD_ZOMBIE)
			{
				// dump_run_queue();
				int64_t pid = child_thread->pid;
				DEBUG("wait thread kfree child: %d\n", pid);
				free_child_thread(child_thread);

				list_del_entry(curr_child_node);
				kernel_unlock_interrupt();
				return pid;
			}
		}
	}
	kernel_unlock_interrupt();
	return 0;
}

void thread_exit()
{
	// thread cannot deallocate the stack while still using it, wait for someone to recycle it.
	// In this lab, idle thread handles this task, instead of parent thread.
	kernel_lock_interrupt();
	DEBUG("thread %d exit\n", curr_thread->pid);
	curr_thread->status = THREAD_ZOMBIE;
	list_del_entry((list_head_t *)curr_thread); // remove from run queue, still in parent's child list
	kernel_unlock_interrupt();
	schedule();
}

void thread_exit_by_pid(int64_t pid)
{
	// thread cannot deallocate the stack while still using it, wait for someone to recycle it.
	// In this lab, idle thread handles this task, instead of parent thread.
	kernel_lock_interrupt();
	DEBUG("thread %d exit\n", pid);
	thread_t *t = threads[pid];
	t->status = THREAD_ZOMBIE;
	list_del_entry((list_head_t *)t); // remove from run queue, still in parent's child list
	kernel_unlock_interrupt();
}

void schedule_timer()
{
	uint64_t cntfrq_el0;
	__asm__ __volatile__("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq_el0));
	// 32 * default timer -> trigger next schedule timer
	add_timer_by_tick(cntfrq_el0 >> 5, adapter_schedule_timer, NULL);
	need_to_schedule = 1;
}

thread_t *thread_create(void *start, char *name)
{
	kernel_lock_interrupt();
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
		ERROR("no available pid, fork error\n");
		kernel_unlock_interrupt();
		return NULL;
	}
	else
	{
		pid_history = new_pid;
	}
	r = (thread_t *)kmalloc(sizeof(thread_t));
	DEBUG("new_pid: %d, thread address: 0x%x\n", new_pid, r);
	init_thread_signal(r);
	threads[new_pid] = r;
	r->name = name;
	r->pid = new_pid;
	r->ppid = curr_thread->pid;
	r->child_list = (child_node_t *)kmalloc(sizeof(child_node_t));
	INIT_LIST_HEAD((list_head_t *)r->child_list);
	r->vma_list = (vm_area_struct_t *)kmalloc(sizeof(vm_area_struct_t));
	INIT_LIST_HEAD((list_head_t *)r->vma_list);
	r->status = THREAD_READY;
	r->user_stack_bottom = kmalloc(USTACK_SIZE);
	DEBUG("new_pid: %d, user_stack_bottom: 0x%x\n", new_pid, r->user_stack_bottom);
	r->kernel_stack_bottom = kmalloc(KSTACK_SIZE);
	DEBUG("new_pid: %d, kernel_stack_bottom: 0x%x\n", new_pid, r->kernel_stack_bottom);
	r->code = start;
	r->context.lr = (uint64_t)start;
	r->context.sp = (uint64_t)r->kernel_stack_bottom + KSTACK_SIZE;
	DEBUG("new_pid: %d, context.sp: 0x%x\n", new_pid, r->context.sp);
	r->context.fp = r->context.sp; // frame pointer for local variable, which is also in stack.

	r->context.pgd = KERNEL_VIRT_TO_PHYS(kmalloc(PAGE_FRAME_SIZE));
    memset(PHYS_TO_KERNEL_VIRT(r->context.pgd), 0, PAGE_FRAME_SIZE);

	child_node_t *child = (child_node_t *)kmalloc(sizeof(child_node_t));
	child->pid = new_pid;
	list_add_tail((list_head_t *)child, (list_head_t *)curr_thread->child_list);

	DEBUG("add new thread: %d, run_que: 0x%x\n", r->pid, run_queue);
	list_add((list_head_t *)r, run_queue);

	DEBUG("add new thread: %d\n", r->pid);
	kernel_unlock_interrupt();
	return r;
}

int8_t has_child(thread_t *thread)
{
	return !list_empty((list_head_t *)thread->child_list);
}

void schedule()
{
	kernel_lock_interrupt();
	thread_t *prev_thread = curr_thread;
	do
	{
		curr_thread = (thread_t *)(((list_head_t *)curr_thread)->next);
		// DEBUG("%d: %s -> %d: %s\n", prev_thread->pid, prev_thread->name, curr_thread->pid, curr_thread->name);
	} while (list_is_head((list_head_t *)curr_thread, run_queue)); // find a runnable thread
	curr_thread->status = THREAD_RUNNING;
	kernel_unlock_interrupt();
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
	}
	INFO("%s exit\n", curr_thread->name);
	thread_exit();
}

void dump_run_queue()
{
	// DEBUG("curr_thread->pid: %d, run_queue: 0x%x, run_queue->next: 0x%x, &run_queue->next: 0x%x, run_queue->prev: 0x%x\n", curr_thread->pid, run_queue, run_queue->next, &(run_queue->next), run_queue->prev);
	// uart_puts("---------------------- run queue list for each ----------------------\r\n");
	// list_head_t *curr;
	// list_for_each(curr, (list_head_t *)run_queue)
	// {
	// 	uart_puts("thread: 0x%x, thread->next: 0x%x, thread->prev: 0x%x, thread->name: %s, thread->pid: %d, thread->ppid: %d", curr, curr->next, curr->prev, ((thread_t *)curr)->name, ((thread_t *)curr)->pid, ((thread_t *)curr)->ppid);
	// 	switch (((thread_t *)curr)->status)
	// 	{
	// 	case THREAD_RUNNING:
	// 		uart_puts(", thread->status: THREAD_RUNNING\r\n");
	// 		break;
	// 	case THREAD_READY:
	// 		uart_puts(", thread->status: THREAD_READY\r\n");
	// 		break;
	// 	case THREAD_BLOCKED:
	// 		uart_puts(", thread->status: THREAD_BLOCKED\r\n");
	// 		break;
	// 	case THREAD_ZOMBIE:
	// 		uart_puts(", thread->status: THREAD_ZOMBIE\r\n");
	// 		break;
	// 	}
	// }
	// uart_puts("--------------------------------- end -------------------------------\r\n");
	recursion_run_queue(threads[1], 0);
}

void recursion_run_queue(thread_t *root, int64_t level)
{
	for (int i = 0; i < level; i++)
		printf("   ");
	printf(" |---");
	dump_thread_info(root);
	list_head_t *curr;
	list_for_each(curr, (list_head_t *)root->child_list)
	{
		// INFO("child: %d\n", child_node_to_thread((child_node_t *)curr)->pid);
		recursion_run_queue(child_node_to_thread((child_node_t *)curr), level + 1);
		// ERROR("OVER");
	}
}

void dump_thread_info(thread_t *thread)
{
	printf("%s\t%d\t%d\t", thread->name, thread->pid, thread->ppid);
	switch (thread->status)
	{
	case THREAD_RUNNING:
		printf("RUNNING\r\n");
		break;
	case THREAD_READY:
		printf("READY\r\n");
		break;
	case THREAD_BLOCKED:
		printf("BLOCKED\r\n");
		break;
	case THREAD_ZOMBIE:
		printf("ZOMBIE\r\n");
		break;
	}
}

void dump_child_thread(thread_t *thread)
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