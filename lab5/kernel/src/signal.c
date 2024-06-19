#include "signal.h"
#include "sched.h"
#include "syscall.h"
#include "memory.h"
#include "list.h"
#include "mini_uart.h"
#include "utils.h"

extern thread_t *curr_thread;
extern thread_t *threads[];

typedef void (*func_ptr)();

func_ptr get_signal_handler_frome_thread(thread_t *thread, int signal) {
	return thread->signal.handler_table[signal];
}

signal_node_t *get_pending_signal_node_from_thread(thread_t *thread) {
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
	// uart_puts("kmalloc pending_list 0x%x\n", thread->signal.pending_list);
	INIT_LIST_HEAD((list_head_t *)thread->signal.pending_list);
}

void kernel_lock_signal(thread_t *thread) {
	thread->signal.lock = 1;
}

void kernel_unlock_signal(thread_t *thread) {
	thread->signal.lock = 0;
}

int8_t signal_is_lock() {
	return curr_thread->signal.lock;
}

void signal_send(int pid, int signal) {
	lock();
	thread_t *target_thread = threads[pid];
	if (target_thread == NULL) {
		// uart_puts("No such thread\n");
		return;
	}
	signal_node_t *new_node = kmalloc(sizeof(signal_node_t));
	new_node->signal = signal;
	// uart_puts("add pending signal %d to process %d\n", signal, pid);
	list_add_tail((list_head_t *)new_node, (list_head_t *)(target_thread->signal.pending_list));
	// uart_puts("--------------------------------------------------------------------------------------\r\n");
	// uart_puts("Pending list of process %d, signal %d, pending prev: 0x%x, pending next: 0x%x\n", pid, new_node->signal, new_node->listhead.prev, new_node->listhead.next);
	// uart_puts("--------------------------------------------------------------------------------------\r\n");
	unlock();
}

void signal_register_handler(int signal, void (*handler)())
{
	uart_puts("register: signal %d, handler 0x%x\n", signal, handler);
	curr_thread->signal.handler_table[signal] = handler;
}

void signal_default_handler()
{
	uart_puts("No handler for this signal\n");
}

void signal_killsig_handler()
{
	uart_puts("Kill handler\n");
	// thread_exit();
	CALL_SYSCALL(SYSCALL_EXIT);
}

int8_t has_pending_signal()
{
	return !list_empty((list_head_t *)(curr_thread->signal.pending_list));
}

void run_pending_signal() {
	lock();
	// uart_puts("Run pending signal\n");
	if (signal_is_lock()) {
		// uart_puts("Signal is locked\n");
		unlock();
		return;
	}
	kernel_lock_signal(curr_thread);
	unlock();

	store_context(&curr_thread->signal.saved_context);

	lock();
	if (!has_pending_signal()){
		// uart_puts("No pending signal\n");
		kernel_unlock_signal(curr_thread);
		unlock();
		return;
	}
	signal_node_t *curr = get_pending_signal_node_from_thread(curr_thread);
	int64_t signal = curr->signal;
	// uart_puts("Run pending signal %d, next: 0x%x, prev: 0x%x\n", signal, curr->listhead.next, curr->listhead.prev);
	list_del_entry((list_head_t *)curr);
	unlock();
	kfree(curr);
	run_signal(signal);
}

void run_signal(int signal) {
	// uart_puts("Run signal %d\n", signal);
	void (*signal_handler)() = get_signal_handler_frome_thread(curr_thread, signal);
	// run registered handler in userspace
	curr_thread->signal.signal_stack_base = kmalloc(USTACK_SIZE);
	JUMP_TO_USER_SPACE(signal_handler_wrapper, signal_handler, curr_thread->signal.signal_stack_base + USTACK_SIZE, NULL);
}

void signal_handler_wrapper(char *dest) {
	// uart_puts("Signal handler wrapper\n");
	run_user_task_wrapper(dest);
	// system call sigreturn
	CALL_SYSCALL(SYSCALL_SIGNAL_RETURN);
}

void signal_return()
{
	// uart_puts("Signal return\n");
	kfree((void *)curr_thread->signal.signal_stack_base);
	load_context(&curr_thread->signal.saved_context);
}

void signal_copy(thread_t* dest_thread, thread_t* src_thread) {                                                         \
    do{                                                                                              
        dest_thread->signal = src_thread->signal;                                                    
        // uart_puts("Copy signal from %d to %d\n", src_thread->pid, dest_thread->pid);                     
        dest_thread->signal.pending_list = kmalloc(sizeof(signal_node_t));                           
        // uart_puts("kmalloc pending_list 0x%x\n", dest_thread->signal.pending_list);                      
        INIT_LIST_HEAD((list_head_t *)dest_thread->signal.pending_list);                             
        // uart_puts("INIT_LIST_HEAD pending_list\n");                                                      
        list_head_t *curr;                                                                           
        list_for_each(curr, (list_head_t *)src_thread->signal.pending_list) {                                                                                            
            signal_node_t *new_node = kmalloc(sizeof(signal_node_t));                                
            // uart_puts("kmalloc new_node 0x%x\n", new_node);                                              
            new_node->signal = ((signal_node_t *)curr)->signal;                                      
            // uart_puts("Copy signal 0x%x -> 0x%x\n", ((signal_node_t *)curr)->signal, new_node->signal);  
            list_add_tail((list_head_t *)new_node, (list_head_t *)dest_thread->signal.pending_list); 
        }                                                                                            
    }while(0);
}