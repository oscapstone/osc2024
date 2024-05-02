#include <stddef.h>
#include "sched.h"
#include "uart1.h"
#include "exception.h"
#include "memory.h"
#include "timer.h"
#include "signal.h"
#include "shell.h"

list_head_t *run_queue;

thread_t threads[PIDMAX + 1];
thread_t *curr_thread = NULL;

int pid_history = 0;
int finish_init_thread_sched = 0;
void init_thread_sched()
{
    lock();
    // init thread freelist and run_queue
    run_queue = init_malloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);

    //init pids
    for (int i = 0; i <= PIDMAX; i++)
    {
        threads[i].isused = 0;
        threads[i].pid = i;
        threads[i].iszombie = 0;
    }

    curr_thread = thread_create(idle, IDLE_PRIORITY);
    thread_create(cli_cmd, SHELL_PRIORITY);

    asm volatile("msr tpidr_el1, %0" ::"r"(curr_thread + sizeof(list_head_t)));
    finish_init_thread_sched = 1;
    add_timer(schedule_timer, 1, "", 0); // start scheduler

    unlock();
}

void idle(){
    while(1)
    {
        kill_zombies();   //reclaim threads marked as DEAD
        el1_interrupt_enable();
        schedule();
    }
}
void add_task_to_runqueue(thread_t *t)
{    
    lock();
    list_head_t *index_threrad;
    list_for_each(index_threrad, run_queue)
    {
        if (((thread_t*)index_threrad)->priority > curr_thread->priority)
        {
            list_add(&t->listhead, index_threrad->prev);
            break;
        }
    }

    if (list_is_head(index_threrad, run_queue))
    {
        list_add_tail(&t->listhead, run_queue);
    }
    unlock();
}
void schedule(){
    // lock();
    list_del_entry(&curr_thread->listhead);
    add_task_to_runqueue(curr_thread);
    curr_thread = (thread_t *)run_queue->next;
    
    while(curr_thread->iszombie)
    {
        curr_thread = (thread_t *)curr_thread->listhead.next;
    }
    switch_to(get_current(), &curr_thread->context);
    // unlock();
}

void kill_zombies(){
    lock();
    list_head_t *curr;
    list_for_each(curr, run_queue)
    {
        if (((thread_t *)curr)->iszombie)
        {
            list_del_entry(curr);
            kfree(((thread_t *)curr)->stack_alloced_ptr);        // free stack
            kfree(((thread_t *)curr)->kernel_stack_alloced_ptr); // free stack
            //kfree(((thread_t *)curr)->data);                   // Don't free data because children may use data
            ((thread_t *)curr)->iszombie = 0;
            ((thread_t *)curr)->isused = 0;
        }
    }

    unlock();
}

int exec_thread(char *data, unsigned int filesize)
{
    thread_t *t_thread = thread_create(run_user_code, NORMAL_PRIORITY);
    lock();
    t_thread->data = kmalloc(filesize);
    t_thread->datasize = filesize;

    // copy file into data
    for (int i = 0; i < filesize;i++)
    {
        t_thread->data[i] = data[i];
    }
    curr_thread = t_thread;
    unlock();
    switch_to(get_current(), &t_thread->context);

    return 0;
}

void run_user_code()
{
    asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
        "msr elr_el1, %1\n\t"   // When el0 -> el1, store return address for el1 -> el0
        "msr spsr_el1, xzr\n\t" // Enable interrupt in EL0 -> Used for thread scheduler
        "msr sp_el0, %2\n\t"    // el0 stack pointer for el1 process
        "mov sp, %3\n\t"        // sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
        "eret\n\t" ::"r"(&curr_thread->context),"r"((unsigned long)curr_thread->data), "r"(curr_thread->context.sp), "r"(curr_thread->kernel_stack_alloced_ptr + KSTACK_SIZE));

}
thread_t *thread_create(void *start, int priority)
{
    lock();
    thread_t *r;
    // find usable PID, don't use the previous one
    if( pid_history > PIDMAX ) return 0;
    if (!threads[pid_history].isused){
        r = &threads[pid_history];
        r->pid = pid_history;
        pid_history += 1;
    }
    else return 0;
    // if (priority < 0 || priority % 10 != 0)
    // {
    //     uart_sendline("Invalid priority\n");
    //     unlock();
    //     return 0;
    // }

    r->iszombie = 0;
    r->priority = priority;
    r->isused = 1;
    r->context.lr = (unsigned long long)start; // current thread's return address
    r->stack_alloced_ptr = kmalloc(USTACK_SIZE);
    r->kernel_stack_alloced_ptr = kmalloc(KSTACK_SIZE);
    r->context.sp = (unsigned long long )r->stack_alloced_ptr + USTACK_SIZE;
    r->context.fp = r->context.sp; // frame pointer for local variable, which is also in stack.

    r->signal_inProcess = 0;
    //initial all signal handler with signal_default_handler (kill thread)
    for (int i = 0; i < SIGNAL_MAX;i++)
    {
        r->signal_handler[i] = signal_default_handler;
        r->sigcount[i] = 0;
    }

    // list_add(&r->listhead, run_queue[priority / 10]);
    add_task_to_runqueue(r);
    // uart_sendline("Thread %d created, lr: 0x%x\n", r->pid, r->context.lr);
    unlock();
    return r;
}

void thread_exit(){
    // thread cannot deallocate the stack while still using it, wait for someone to recycle it.
    // In this lab, idle thread handles this task, instead of parent thread.
    lock();
    uart_sendline("Thread %d exit\n", curr_thread->pid);
    curr_thread->iszombie = 1;
    unlock();
    schedule();
}

void schedule_timer(char* notuse){
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0));
    add_timer(schedule_timer, cntfrq_el0 >> 5, "", 1);
    // 32 * default timer -> trigger next schedule timer
}

void foo(){
    // Lab5 Basic 1 Test function
    for (int i = 0; i < 10; ++i)
    {
        uart_sendline("Thread id: %d, Run %d time\n", curr_thread->pid, i);
        int r = 1000000;
        while (r--) { asm volatile("nop"); }
        schedule();
    }
    thread_exit();
}
