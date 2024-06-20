#include "memory.h"
#include "timer.h"
#include "utility.h"
#include "stdint.h"
#include "syscall.h"
#include "exception.h"
#include "mini_uart.h"
#include "shell.h"
#include "sched.h"

list_head_t *run_queue;

thread_t *threads[MAX_PID + 1];
thread_t *curr_thread;

extern char __begin;
extern char __end;

static int64_t pid_history = 0;
int8_t need_to_schedule = 0;

static inline int8_t in_kernel_img_space(uint64_t addr)
{
	return addr >= &__begin && addr < & __end;
}
thread_t *init_idle_thread(void *code, char *name, signed long pid, signed long ppid)
{
    thread_t *thread = (thread_t *)kmalloc(sizeof(thread_t));

    threads[0] = thread;
    thread->name = name;
    thread->pid = pid;
    thread->ppid = ppid;
    thread->code = code;
    thread->child_list = (child_node_t *)kmalloc(sizeof(child_node_t));
    INIT_LIST_HEAD((list_head_t *)thread->child_list);

    thread->status = THREAD_IS_READY;
    thread->user_stack_base = kmalloc(USTACK_SIZE);
    thread->kernel_stack_base = kmalloc(KSTACK_SIZE);
    thread->context.lr = (unsigned long)code;
    thread->context.sp = (unsigned long)thread->kernel_stack_base + KSTACK_SIZE;
    thread->context.fp = thread->context.sp;
    list_add((list_head_t *)thread, run_queue);
    return thread;
}
inline int8_t thread_code_can_free(thread_t *thread)
{
	return !in_kernel_img_space((uint64_t)thread->code);
}

static inline thread_t *child_node_to_thread(child_node_t *node)
{
	return threads[(node)->pid];
}

static inline int free_child_thread(thread_t *child_thread)
{
	list_head_t *curr;
	list_head_t *n;
	//("free child thread: %d\n", child_thread->pid);

	list_for_each_safe(curr, n, (list_head_t *)child_thread->child_list)
	{
		thread_t *curr_child = child_node_to_thread((child_node_t *)curr);
		curr_child->ppid = 1; // assign to init process
		//("move child thread: %d to init process\n", curr_child->pid);
		list_del_entry(curr);
		list_add_tail(curr, (list_head_t *)threads[1]->child_list);
	}

	if (thread_code_can_free(child_thread))
	{
		kfree(child_thread->code);
	}
	threads[child_thread->pid] = NULL;
	kfree(child_thread->child_list);
	kfree(child_thread->user_stack_base);
	kfree(child_thread->kernel_stack_base);
	kfree(child_thread->name);
	block();
	kfree(child_thread);
	//("child_thread: 0x%x, curr_thread: 0x%x\n", child_thread, curr_thread);
	// dump_run_queue();
	return 0;
}

void init_thread_sched()
{
    lock();

    run_queue = kmalloc(sizeof(thread_t));
    INIT_LIST_HEAD(run_queue);

    // idle process?
    char *thread_name = kmalloc(5);
    strcpy(thread_name, "idle");
    thread_t *idle_thread = init_idle_thread(idle, thread_name, 0, 0);
    set_current_thread_context(&(idle_thread->context));
    curr_thread = idle_thread;
    //init process
    thread_name = kmalloc(5);
	strcpy(thread_name, "init");
	thread_t * init_thread = thread_create(init,thread_name );
	init_thread->datasize = 0x4000;
	curr_thread = init_thread;

    // foo process
    thread_name = kmalloc(5);
    strcpy(thread_name, "foo");
    thread_t *foo_thread = thread_create(foo,thread_name );
    foo_thread->datasize = 0x4000;
    // curr_thread = init_thread;

    // foo process
    thread_name = kmalloc(5);
    strcpy(thread_name, "foo2");
    thread_t *foo2_thread = thread_create(foo,thread_name );
    foo2_thread->datasize = 0x4000;


    // kernel shell process
    thread_name = kmalloc(7);
    strcpy(thread_name, "kshell");
    thread_t *kshell = thread_create( shell,thread_name);
    kshell->datasize = 0x100000;
    curr_thread = idle_thread;

    // uart_puts("Shell init \r\n");
    unlock();
}

void init()
{
	while (1)
	{
		int64_t pid = wait();
	}
}

thread_t *thread_create(void *code,char *name )
{
    lock();
    thread_t *t;
    int64_t new_pid = -1;

    for (int i = 1; i < MAX_PID; i++)
    {
        if (threads[pid_history + i] == NULL)
        {
            new_pid = pid_history + i;
            break;
        }
    }
    if (new_pid == -1)
    {
        uart_puts("No Available PID \n");
        unlock();
        return NULL;
    }
    else
    {
        pid_history = new_pid;
    }
    t = (thread_t *)kmalloc(sizeof(thread_t));
	// init_thread_signal(t);
    // signal?
    threads[new_pid] = t;
    t->name = name;
    t->pid = new_pid;
    t->ppid = curr_thread->pid;
    t->child_list = (child_node_t *)kmalloc(sizeof(child_node_t));
    INIT_LIST_HEAD((list_head_t *)t->child_list);
    t->status = THREAD_IS_READY;
    t->user_stack_base = kmalloc(USTACK_SIZE);
    t->kernel_stack_base = kmalloc(KSTACK_SIZE);
    t->code = code;
    t->context.lr = (uint64_t)code;
    t->context.sp = (uint64_t)t->kernel_stack_base + KSTACK_SIZE;
    t->context.fp = t->context.sp;

    child_node_t *child = (child_node_t *)kmalloc(sizeof(child_node_t));
    child->pid = new_pid;
    list_add_tail((list_head_t *)child, (list_head_t *)curr_thread->child_list);
    list_add((list_head_t *)t, run_queue);
    uart_puts("[+] Add a thread \r\n");

    unlock();
    return t;
}

int64_t wait()
{
	lock();
    uart_puts("wait thread \r\n");
	curr_thread->status = THREAD_IS_BLOCKED;
	while (1)
	{
		unlock();
		schedule();
		lock();
		struct list_head *curr_child_node;
		list_head_t *n;
		list_for_each_safe(curr_child_node, n, (list_head_t *)curr_thread->child_list)
		{
			thread_t *child_thread = child_node_to_thread((child_node_t *)curr_child_node);
			if (child_thread->status == THREAD_IS_ZOMBIE)
			{
				// dump_run_queue();
				int64_t pid = child_thread->pid;
				//("wait thread kfree child: %d\n", pid);
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

void foo()
{
    uart_puts(curr_thread->name);
    uart_puts("\r\n");
    // Lab5 Basic 1 Test function
    for (int i = 0; i < 10; ++i)
    {
        uart_puts("Thread id: ");
        put_int(curr_thread->pid);
        uart_puts(" ");
        put_int(i);
        uart_puts("\r\n");
        // schedule();
        // delay();
    }
    thread_exit();
}
void schedule_timer()
{
    // uart_puts("[+] Schedule timer \r\n");
    uint64_t cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq_el0));
    // 32 * default timer -> trigger next schedule timer
    // put_int(cntfrq_el0 / 0x10000000);
    add_timer(schedule_timer, cntfrq_el0 >> 5, NULL);
    need_to_schedule = 1;
}
void schedule()
{
    // 執行下一個 thread
    //  current thread 換成 run queue->next
    lock();

    thread_t *prev_thread = curr_thread;
    do
    {
        curr_thread = (thread_t *)(((list_head_t *)curr_thread)->next);
    } while (list_is_head((list_head_t *)curr_thread, run_queue)); // find a runnable thread

    curr_thread->status = THREAD_IS_RUNNING;
    // uart_puts("[+] Scheduler \r\n");
    unlock();
    switch_to(get_current_thread_context(), &(curr_thread->context));
}
void idle()
{
    // 當schedule 沒東西時候 執行此process
    //  while True:
    //     kill_zombies() # reclaim threads marked as DEAD
    //     schedule() # switch to any other runnable thread
    unlock();
    uart_puts("idle process\r\n");
    schedule_timer();

    while (1)
    {
        // uart_puts(" in idle process\r\n");
        schedule();
    }
};

void thread_exit()
{
    uart_puts("[-] thread exit \r\n");
    lock();
    // 將current thread設成zombie
    curr_thread->status = THREAD_IS_ZOMBIE;
    // 從 run queue移除
    list_del_entry((list_head_t *)curr_thread);
    // 呼叫schedule執行下一個thread
    unlock();
    schedule();
};

void thread_exit_by_pid(int64_t pid)
{
	// thread cannot deallocate the stack while still using it, wait for someone to recycle it.
	// In this lab, idle thread handles this task, instead of parent thread.
	lock();
	// DEBUG("thread %d exit\n", pid);
	thread_t *t = threads[pid];
	t->status = THREAD_IS_ZOMBIE;
	list_del_entry((list_head_t *)t); // remove from run queue, still in parent's child list
	unlock();
}
void dump_thread_info(thread_t *thread)
{
	// printf("%s\t%d\t%d\t", thread->name, thread->pid, thread->ppid);
	uart_puts("thread name : ");
    uart_puts(thread->name);
	uart_puts("  thread pid :  ");
    put_int(thread->pid);
	uart_puts(" thread ppid :  ");
    put_int(thread->ppid);
	switch (thread->status)
	{
	case THREAD_IS_RUNNING:
		uart_puts(" is RUNNING\r\n");
		break;
	case THREAD_IS_READY:
		uart_puts(" is READY\r\n");
		break;
	case THREAD_IS_BLOCKED:
		uart_puts(" is BLOCKED\r\n");
		break;
	case THREAD_IS_ZOMBIE:
		uart_puts(" is ZOMBIE\r\n");
		break;
	}
}
void recursion_run_queue(thread_t *root, int64_t level)
{
	for (int i = 0; i < level; i++)
		uart_puts("   ");
	uart_puts(" |---");
	dump_thread_info(root);
	list_head_t *curr;
	list_for_each(curr, (list_head_t *)root->child_list)
	{
		// INFO("child: %d\n", child_node_to_thread((child_node_t *)curr)->pid);
		recursion_run_queue(child_node_to_thread((child_node_t *)curr), level + 1);
		// ERROR("OVER");
	}
}
void dump_run_queue()
{
	recursion_run_queue(threads[1], 0);
}

