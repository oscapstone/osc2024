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
thread_t *curr_thread;

int pid_history = 0;

void init_thread_sched()
{
    lock();
    // init thread freelist and run_queue
    run_queue = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);

    //init pids
    for (int i = 0; i <= PIDMAX; i++)
    {
        threads[i].isused = 0;
        threads[i].pid = i;
        threads[i].iszombie = 0;
    }

    // asm volatile("msr tpidr_el1, %0" ::"r"(kmalloc(sizeof(thread_t)))); // Don't let thread structure NULL as we enable the functionality

    thread_t* idlethread = thread_create(idle);
    curr_thread = idlethread;
    asm volatile("msr tpidr_el1, %0" ::"r"(curr_thread + sizeof(list_head_t))); 
    unlock();
}

void idle(){
    while(1)
    {
        kill_zombies();   //reclaim threads marked as DEAD
        schedule();       //switch to next thread in run queue
        if (list_size(run_queue) == 1)
        {
            do_cmd_reboot(NULL, 0);
        }
    }
}

void schedule(){
    lock();
    do {
        curr_thread = (thread_t *)curr_thread->listhead.next;
    } while (list_is_head(&curr_thread->listhead, run_queue)); // find a runnable thread
    if (list_size(run_queue) != 1){
        switch_to(get_current(), &curr_thread->context);
    }
    unlock();
}

void kill_zombies(){
    lock();
    list_head_t *curr;
    list_for_each(curr,run_queue)
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
    thread_t *t = thread_create(data);
    t->data = kmalloc(filesize);
    t->datasize = filesize;
    t->context.lr = (unsigned long)t->data; // set return address to program if function call completes
    // copy file into data
    for (int i = 0; i < filesize;i++)
    {
        t->data[i] = data[i];
    }

    curr_thread = t;
    add_timer(schedule_timer, 1, "", 0); // start scheduler
    // eret to exception level 0
    asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
        "msr elr_el1, %1\n\t"   // When el0 -> el1, store return address for el1 -> el0
        "msr spsr_el1, xzr\n\t" // Enable interrupt in EL0 -> Used for thread scheduler
        "msr sp_el0, %2\n\t"    // el0 stack pointer for el1 process
        "mov sp, %3\n\t"        // sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
        "eret\n\t" ::"r"(&t->context),"r"(t->context.lr), "r"(t->context.sp), "r"(t->kernel_stack_alloced_ptr + KSTACK_SIZE));

    return 0;
}

thread_t *thread_create(void *start)
{
    lock();
    thread_t *r;
    // find usable PID, don't use the previous one
    if( pid_history > PIDMAX ) return 0;
    if (!threads[pid_history].isused){
        r = &threads[pid_history];
        pid_history += 1;
    }
    else return 0;

    r->iszombie = 0;
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
    list_add(&r->listhead, run_queue);
    unlock();
    return r;
}

void thread_exit(){
    // thread cannot deallocate the stack while still using it, wait for someone to recycle it.
    // In this lab, idle thread handles this task, instead of parent thread.
    lock();
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
