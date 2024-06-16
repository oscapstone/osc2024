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

    // init process
    thread_name = kmalloc(5);
    strcpy(thread_name, "foo");
    thread_t *init_thread = thread_create(thread_name, foo);
    init_thread->datasize = 0x4000;
    // curr_thread = init_thread;

    // init process
    thread_name = kmalloc(5);
    strcpy(thread_name, "foo2");
    thread_t *foo_2 = thread_create(thread_name, foo);
    foo_2->datasize = 0x4000;

    // curr_thread = idle_thread;

    // kernel shell process
    thread_name = kmalloc(7);
    strcpy(thread_name, "kshell");
    thread_t *kshell = thread_create( thread_name, shell);
    kshell->datasize = 0x100000;
    curr_thread = idle_thread;

    // uart_puts("Shell init \r\n");
    unlock();
}

thread_t *thread_create(char *name, void *code)
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
    // signal?
    threads[new_pid] = t;
    t->name = name;
    // uart_puts("thread create:");
    // uart_puts(t->name);
    // uart_puts("\r\n");
    t->pid = new_pid;
    // uart_puts("thread pid:");
    // put_int(t->pid);
    // uart_puts("\r\n");
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
    list_add_tail((list_head_t *)t, run_queue);
    uart_puts("[+] Add a thread \r\n");

    unlock();
    return t;
}

int64_t wait()
{
	lock();
	//("block thread: %d\n", curr_thread->pid);
	curr_thread->status = THREAD_IS_BLOCKED;
	while (1)
	{
		// //("wait thread: %d\n", curr_thread->pid);
		unlock();
		schedule();
		lock();
		// //_BLOCK({
		// 	dump_child_thread(curr_thread);
		// });
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
        delay();
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
    add_timer(schedule_timer, 1, NULL);
    need_to_schedule = 1;
}
void schedule()
{
    // 執行下一個 thread
    //  current thread 換成 run queue->next
    lock();
    // uart_puts("[+] Scheduler \r\n");

    thread_t *prev_thread = curr_thread;
    do
    {
        curr_thread = (thread_t *)(((list_head_t *)curr_thread)->next);
        // put_int(prev_thread->pid);
        // uart_puts("\r\n");
        // uart_puts(prev_thread->name);
        // uart_puts("\r\n");
        // put_int(curr_thread->pid);
        // uart_puts("\r\n");
        // uart_puts(curr_thread->name);
        // uart_puts("\r\n");
    } while (list_is_head((list_head_t *)curr_thread, run_queue)); // find a runnable thread

    curr_thread->status = THREAD_IS_RUNNING;
    unlock();
    // uart_puts("[+] switching \r\n");
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


