#include "debug.h"
#include "signal.h"
#include "sched.h"
#include "syscall.h"
#include "memory.h"
#include "list.h"

extern thread_t *curr_thread;
extern thread_t *threads[];

typedef void (*func_ptr)();
static inline func_ptr get_signal_handler_frome_thread(thread_t *thread, int signal)
{
	return thread->signal.handler_table[signal];
}

static inline signal_node_t *get_pending_signal_node_from_thread(thread_t *thread)
{
	return (signal_node_t *)(((list_head_t *)(thread->signal.pending_list))->next);
}

void init_thread_signal(thread_t *thread)
{
	for (int i = 0; i < MAX_SIGNAL; i++)
	{
		thread->signal.handler_table[i] = signal_default_handler;
	}
	thread->signal.handler_table[SIGKILL] = signal_killsig_handler;
	thread->signal.lock = 0;
	thread->signal.pending_list = kmalloc(sizeof(signal_node_t));
	DEBUG("kmalloc pending_list 0x%x\n", thread->signal.pending_list);
	INIT_LIST_HEAD((list_head_t *)thread->signal.pending_list);
}

void kernel_lock_signal(thread_t *thread)
{
	// DEBUG("Lock signal, pid: %d\n", thread->pid);
	thread->signal.lock = 1;
}

void kernel_unlock_signal(thread_t *thread)
{
	// DEBUG("Unlock signal, pid: %d\n", thread->pid);
	thread->signal.lock = 0;
}

int8_t signal_is_lock()
{
	// DEBUG("signal_lock: %d\n", curr_thread->signal.lock);
	return curr_thread->signal.lock;
}

void signal_send(int pid, int signal)
{
	kernel_lock_interrupt();
	thread_t *target_thread = threads[pid];
	if (target_thread == NULL)
	{
		ERROR("No such thread\n");
		return;
	}
	signal_node_t *new_node = kmalloc(sizeof(signal_node_t));
	new_node->signal = signal;
	DEBUG("add pending signal %d to process %d\n", signal, pid);
	list_add_tail((list_head_t *)new_node, (list_head_t *)(target_thread->signal.pending_list));
	DEBUG("--------------------------------------------------------------------------------------\r\n");
	DEBUG("Pending list of process %d, signal %d, pending prev: 0x%x, pending next: 0x%x\n", pid, new_node->signal, new_node->listhead.prev, new_node->listhead.next);
	DEBUG("--------------------------------------------------------------------------------------\r\n");
	kernel_unlock_interrupt();
}

void signal_register_handler(int signal, void (*handler)())
{
	DEBUG("register: signal %d, handler 0x%x\n", signal, handler);
	curr_thread->signal.handler_table[signal] = handler;
}

void signal_default_handler()
{
	WARNING("No handler for this signal\n");
}

void signal_killsig_handler()
{
	DEBUG("Kill handler\n");
	// thread_exit();
	CALL_SYSCALL(SYSCALL_EXIT);
}

int8_t has_pending_signal()
{
	return !list_empty((list_head_t *)(curr_thread->signal.pending_list));
}

void run_pending_signal()
{
	kernel_lock_interrupt();
	// DEBUG("Run pending signal\n");
	if (signal_is_lock())
	{
		// DEBUG("Signal is locked\n");
		kernel_unlock_interrupt();
		return;
	}
	kernel_lock_signal(curr_thread);
	kernel_unlock_interrupt();

	store_context(&curr_thread->signal.saved_context);

	kernel_lock_interrupt();
	if (!has_pending_signal()){
		// DEBUG("No pending signal\n");
		kernel_unlock_signal(curr_thread);
		kernel_unlock_interrupt();
		return;
	}
	signal_node_t *curr = get_pending_signal_node_from_thread(curr_thread);
	int64_t signal = curr->signal;
	DEBUG("Run pending signal %d, next: 0x%x, prev: 0x%x\n", signal, curr->listhead.next, curr->listhead.prev);
	list_del_entry((list_head_t *)curr);
	kernel_unlock_interrupt();
	kfree(curr);
	run_signal(signal);
}

void run_signal(int signal)
{
	DEBUG("Run signal %d\n", signal);
	void (*signal_handler)() = get_signal_handler_frome_thread(curr_thread, signal);
	// run registered handler in userspace
	curr_thread->signal.signal_stack_base = kmalloc(USTACK_SIZE);
	JUMP_TO_USER_SPACE(signal_handler_wrapper, signal_handler, curr_thread->signal.signal_stack_base + USTACK_SIZE, NULL);
}

void signal_handler_wrapper(char *dest)
{
	((void (*)(void))dest)();
	// system call sigreturn
	CALL_SYSCALL(SYSCALL_SIGNAL_RETURN);
}

void signal_return()
{
	DEBUG("Signal return\n");
	kfree((void *)curr_thread->signal.signal_stack_base);
	load_context(&curr_thread->signal.saved_context);
}