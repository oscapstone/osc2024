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

static int64_t pid_history = 0;
int8_t need_to_schedule = 0;

thread_t *init_idle_thread(void* code, char *name, signed long pid, signed long ppid){
    thread_t *thread = (thread_t *) kmalloc(sizeof(thread_t));

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
    thread->context.lr = (unsigned long )code;
    thread->context.sp = (unsigned long )thread->kernel_stack_base + KSTACK_SIZE;
    thread->context.fp = thread->context.sp;
    list_add((list_head_t *) thread, run_queue);
    return thread;


}

void init_thread_sched()
{
    lock();

    run_queue = kmalloc(sizeof(thread_t));
    INIT_LIST_HEAD(run_queue);

    // idle process?
    char *thread_name = kmalloc(5);
    strcpy(thread_name, "idle");
    thread_t * idle_thread = init_idle_thread(idle,thread_name, 0,0);
    set_current_thread_context(&(idle_thread->context));
    curr_thread = idle_thread;

    // init process
    thread_name = kmalloc(5);
    strcpy(thread_name, "foo");
    thread_t *init_thread = thread_create(foo, thread_name);
    init_thread->datasize = 0x4000;
    curr_thread = init_thread;

    // kernel shell process
    thread_name = kmalloc(7);
	strcpy(thread_name, "kshell");
	thread_t *kshell = thread_create(shell, thread_name);
	kshell->datasize = 0x100000;
	schedule_timer();
	curr_thread = idle_thread;
	unlock();

};
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
    if (new_pid == -1){
        uart_puts("No Available PID \n");
        unlock();
        return NULL;
    } else {
        pid_history = new_pid;
    }
    t = (thread_t*) kmalloc(sizeof(thread_t));
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
    t->context.lr = (uint64_t) code;
    t->context.sp = (uint64_t) t->kernel_stack_base + KSTACK_SIZE;
    t->context.fp = t->context.sp;

    child_node_t *child = (child_node_t* )kmalloc(sizeof(child_node_t));
    child->pid = new_pid;
    list_add_tail((list_head_t *)child, (list_head_t *)curr_thread->child_list);
    list_add((list_head_t*)t, run_queue);

    unlock();
    return t;



}
void foo(){
    uart_puts("foo\r\n");
    // Lab5 Basic 1 Test function
	for (int i = 0; i < 10; ++i)
	{
		uart_puts("Thread id: ");
        put_int(curr_thread->pid);
        put_int(i);
// #ifdef QEMU
// 		for (int i = 0; i < 10000000; i++) // qemu
// #else
// 		for (int i = 0; i < 100000; i++) // pi
// #endif
// 		{
// 			asm volatile("nop");
// 		}
	}
	// INFO("%s exit\n", curr_thread->name);
	thread_exit();
}
void schedule_timer()
{   
    put_int(22222);
	uint64_t cntfrq_el0;
	__asm__ __volatile__("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq_el0));
	// 32 * default timer -> trigger next schedule timer
	add_timer(cntfrq_el0 >> 5, schedule_timer, NULL);
	need_to_schedule = 1;
}
void schedule(){
    //執行下一個 thread
    // current thread 換成 run queue->next
    lock();
    thread_t *prev_thread = curr_thread;
    while (list_is_head((list_head_t *)curr_thread, run_queue)){
        curr_thread = (thread_t *) (((list_head_t *)curr_thread)->next);
    }
    curr_thread->status = THREAD_IS_RUNNING;
    unlock();
    switch_to(get_current_thread_context(), &(curr_thread->context));

    //
};
void idle(){
    // 當schedule 沒東西時候 執行此process
    //  while True:
    //     kill_zombies() # reclaim threads marked as DEAD
    //     schedule() # switch to any other runnable thread
    unlock();
	uart_puts("idle process\r\n");
	while (1)
	{
		schedule();
        // shell();
	}
};

void thread_exit(){
    lock();
    // 將current thread設成zombie
    curr_thread->status = THREAD_IS_ZOMBIE;
    // 從 run queue移除
    list_del_entry((list_head_t *)curr_thread);
    // 呼叫schedule執行瞎下一個thread
    unlock();
    schedule();
};
