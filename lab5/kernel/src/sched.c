#include "sched.h"
#include "uart1.h"
#include "exception.h"
#include "memory.h"
#include "timer.h"
#include "shell.h"
#include "signal.h"
#include "stdio.h"

list_head_t *run_queue;
execfile c_execfile;

thread_t threads[PIDMAX + 1];
thread_t *curr_thread;

int pid_history = 0;
int timer_sched_flag = 0;
int shell_flag = 0;

void thread_sched_init()
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

    asm volatile("msr tpidr_el1, %0" ::"r"(kmalloc(sizeof(thread_context_t)))); // Don't let thread structure NULL as we enable the functionality
    thread_t *idlethread = thread_create(start_shell);
    curr_thread = idlethread;

    // thread_create(idle);
    unlock();
}

thread_t *thread_create(void *start)
{
    lock();
    thread_t *r;
    // find usable PID, don't use the previous one
    if (pid_history > PIDMAX)
    {
        unlock();
        return 0;
    }

    if (!threads[pid_history].isused)
    {
        r = &threads[pid_history];
        pid_history += 1;
    }
    else
    {
        unlock();
        return 0;
    }

    r->iszombie = 0;
    r->isused = 1;
    r->context.lr = (unsigned long long)start;
    r->stack_alloced_ptr = kmalloc(USTACK_SIZE);
    r->kernel_stack_alloced_ptr = kmalloc(KSTACK_SIZE);
    r->context.sp = (unsigned long long)r->stack_alloced_ptr + USTACK_SIZE;
    r->context.fp = r->context.sp; // frame pointer for local variable, which is also in stack.

    r->signal_inProcess = 0;
    // initial all signal handler with signal_default_handler (kill thread)

    for (int i = 0; i < SIGNAL_MAX; i++)
    {
        r->signal_handler[i] = signal_default_handler;
        r->sigcount[i] = 0;
    }

    list_add_tail(&r->listhead, run_queue);
    unlock();
    return r;
}

void schedule()
{
    lock();
    // uart_sendlinek("Run queue size :%d \n", list_size(run_queue));
    do
    {
        // uart_sendlinek("Run queue size :%d \n", list_size(run_queue));
        curr_thread = (thread_t *)curr_thread->listhead.next;
    } while (list_is_head(&curr_thread->listhead, run_queue)); // find a runnable thread
    unlock();
    // uart_sendlinek("curr_thread :%d \n",curr_thread->pid);
    // uart_sendlinek("curr_thread->context.lr :0x%x \n",curr_thread->context.lr);
    switch_to(get_current(), &curr_thread->context);
}

void idle()
{
    // uart_sendlinek("This is idle\n");
    while (shell_flag > 0)
    {
        // uart_sendlinek("This is idle\n");
        kill_zombies(); // reclaim threads marked as DEAD
        schedule();     // switch to next thread in run queue
        delay(10000);
    }
    kill_zombies();
}

void thread_exit()
{
    // thread cannot deallocate the stack while still using it, wait for someone to recycle it.
    // In this lab, idle thread handles this task, instead of parent thread.
    lock();
    curr_thread->iszombie = 1;
    shell_flag--;
    unlock();
    // delay(10000);
    // schedule();
}

void kill_zombies()
{
    lock();
    list_head_t *curr;
    list_for_each(curr, run_queue)
    {
        if (((thread_t *)curr)->iszombie)
        {
            list_del_entry(curr);
            kfree(((thread_t *)curr)->stack_alloced_ptr);        // free stack
            kfree(((thread_t *)curr)->kernel_stack_alloced_ptr); // free stack
            // kfree(((thread_t *)curr)->data);                   // Don't free data because children may use data
            ((thread_t *)curr)->iszombie = 0;
            ((thread_t *)curr)->isused = 0;
        }
    }
    unlock();
}

void foo()
{
    // Lab5 Basic 1 Test function
    for (int i = 0; i < 10; ++i)
    {
        uart_sendlinek("Thread id: %d %d\n", curr_thread->pid, i);
        int r = 1000000;
        while (r--)
        {
            asm volatile("nop");
        }
        schedule();
    }
    // uart_sendlinek("exit\n");
    thread_exit();
}

int exec_thread()
{
    lock();
    shell_flag++;
    thread_t *t = thread_create(exec_proc);
    curr_thread = t;
    //uart_sendlinek("timer_sched_flag : %d\n", timer_sched_flag);
    if (!timer_sched_flag)
    {
        timer_sched_flag = 1;
        add_timer(schedule_timer, 1, "", setSecond); // start scheduler
    }
    unlock();
    schedule();

    return 0;
}

void exec_proc()
{
    lock();
    char *data = c_execfile.data;
    unsigned int filesize = c_execfile.filesize;
    thread_t *t = curr_thread;

    uart_sendlinek("filesize : %d\n", filesize);
    t->data = kmalloc(filesize);
    t->datasize = filesize;
    t->context.lr = (unsigned long)t->data; // set return address to program if function call completes
    // copy file into data
    for (int i = 0; i < filesize; i++)
    {
        t->data[i] = data[i];
    }
    //unlock();
    uart_sendlinek("curr_thread execproc: %x\n",curr_thread);
    uart_sendlinek("user stack : %x\n", t->context.sp);
    uart_sendlinek("kernel stack : %x\n", (t->kernel_stack_alloced_ptr + KSTACK_SIZE));
    // eret to exception level 0
    //lock();
    asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
        "msr elr_el1, %1\n\t"   // When el0 -> el1, store return address for el1 -> el0
        "msr spsr_el1, xzr\n\t" // EL1h (SPSel = 1) with interrupt disabled
        "msr sp_el0, %2\n\t"    // el0 stack pointer for el1 process
        "mov sp, %3\n\t"        // sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
        "mov x0, %4\n\t"
        ::"r"(&t->context),
        "r"(exec_wrapper), "r"(t->context.sp), "r"(t->kernel_stack_alloced_ptr + KSTACK_SIZE), "r"(t->context.lr));

    asm("eret\n\t");
}

void exec_wrapper(unsigned long long exec_addr)
{
    //uart_sendlinek("in exec_wrapper\n");
    void (*exec_handler)();
    exec_handler = (void (*)())exec_addr;
    // unlock
    uart_sendlinek("exec_handler : %x\n",exec_addr);
    uart_sendlinek("curr_thread : %x\n",curr_thread);
    //uart_sendlinek(curr_thread->context.lr));
    asm("mov x8,514\n\t"
        "svc 0\n\t");
    (exec_handler)();
}

void schedule_timer(char *notuse)
{
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq_el0));
    add_timer(schedule_timer, cntfrq_el0 >> 5, "", setTick);
    // schedule();
    //  32 * default timer -> trigger next schedule timer
}
