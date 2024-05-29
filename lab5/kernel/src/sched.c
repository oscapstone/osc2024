#include "sched.h"
#include "uart1.h"
#include "exception.h"
#include "memory.h"
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

    // init pids
    for (int i = 0; i <= PIDMAX; i++)
    {
        threads[i].isused = 0;
        threads[i].pid = i;
        threads[i].iszombie = 0;
    }

    thread_t *idlethread = thread_create(idle);
    curr_thread = idlethread;
    asm volatile("msr tpidr_el1, %0" ::"r"(&curr_thread->context));
    thread_create(start_shell);
    // unsigned long val;
    // asm volatile("msr tpidr_el1, %0" ::"r"((curr_thread + sizeof(list_head_t)))); //為什麼這樣會過? 因為確實讓prev和next指向同一個thread了，而這代表的是curr_thread + 數個 thread_t 空間的地址
    // asm volatile("mrs %0, tpidr_el1" : "=r" (val));
    // uart_sendline("val2: %x\n", val);
    unlock();
}

void idle()
{
    while (1)
    {
        // uart_sendline("This is idle\n"); // debug
        kill_zombies(); // reclaim threads marked as DEAD
        schedule();     // switch to next thread in run queue
    }
}

void schedule()
{
    lock();
    do
    {
        // uart_sendline("This is in do-while loop curr_thread->pid %d\n", curr_thread->pid);
        curr_thread = (thread_t *)curr_thread->listhead.next;
    } while (list_is_head(&curr_thread->listhead, run_queue) || curr_thread->iszombie == 1); // find a runnable thread
    // uart_sendline("This is curr_thread->pid %d\n", curr_thread->pid);
    unlock();
    switch_to(get_current(), &curr_thread->context);
}

void kill_zombies()
{
    lock();
    list_head_t *curr;
    list_for_each(curr, run_queue)
    {
        if (((thread_t *)curr)->iszombie)
        {
            uart_sendline("This is zombie pid %d\n", ((thread_t *)curr)->pid);
            list_del_entry(curr);
            kfree(((thread_t *)curr)->stack_alloced_ptr);        // free stack
            kfree(((thread_t *)curr)->kernel_stack_alloced_ptr); // free stack
            // kfree(((thread_t *)curr)->data);                   // Don't free data because children may use data
            ((thread_t *)curr)->iszombie = 0;
            ((thread_t *)curr)->isused = 0;
            uart_sendline("kill_zombies\n");
        }
    }
    unlock();
}

thread_t *thread_create(void *start)
{
    lock();
    thread_t *r;
    // find usable PID, don't use the previous one
    if (pid_history > PIDMAX)
        return 0;
    if (!threads[pid_history].isused)
    {
        r = &threads[pid_history];
        pid_history += 1;
        // uart_sendline("This is run_queue->pid %d\n", r->pid);  // for Debug
    }
    else
        return 0;

    r->iszombie = 0;
    r->isused = 1;
    r->context.lr = (unsigned long long)start;
    r->stack_alloced_ptr = kmalloc(USTACK_SIZE);
    r->kernel_stack_alloced_ptr = kmalloc(KSTACK_SIZE);
    r->context.sp = (unsigned long long)r->kernel_stack_alloced_ptr + KSTACK_SIZE;
    r->context.fp = r->context.sp; // frame pointer for local variable, which is also in stack.

    list_add_tail(&r->listhead, run_queue);
    unlock();
    return r;
}

void thread_exit()
{
    // thread cannot deallocate the stack while still using it, wait for someone to recycle it.
    // In this lab, idle thread handles this task, instead of parent thread.
    lock();
    curr_thread->iszombie = 1;
    unlock();
    schedule();
}

void foo()
{
    // Lab5 Basic 1 Test function
    for (int i = 0; i < 3; ++i)
    {
        uart_sendline("foo() thread pid is %d in for loop index = %d\n", curr_thread->pid, i);
        int r = 1000000;
        while (r--)
        {
            asm volatile("nop");
        }
        schedule();
        uart_sendline("In for loop - foo() thread pid is %d in for loop index = %d\n", curr_thread->pid, i);
    }
    thread_exit();
}
