#include "schedule.h"
#include "uart.h"
#include "exception.h"
#include "memory.h"
#include "timer.h"
#include "signal.h"

list_head_t *run_queue; // 等待要執行的

thread_t threads[PIDMAX + 1];
thread_t *curr_thread;

int pid_history = 0;

void init_thread_sched()
{
    disable_irq();

    run_queue = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);

    for (int i = 0; i <= PIDMAX; i++)
    {
        threads[i].isused = 0;
        threads[i].pid = i;
        threads[i].iszombie = 0;
    }

    // https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/TPIDR-EL1--EL1-Software-Thread-ID-Register
    asm volatile("msr tpidr_el1, %0" ::"r"(kmalloc(sizeof(thread_t))));
    // Don't let thread structure NULL as we enable the functionality

    thread_t *idlethread = thread_create(idle);
    curr_thread = idlethread;
    enable_irq();
}

void idle()
{
    while (1)
    {
        kill_zombies(); // reclaim threads marked as DEAD
        schedule();     // switch to any other runnable thread
    }
}

void schedule()
{
    disable_irq();
    // find the next thread in run queue
    thread_t *tmp = curr_thread;
    do
    {
        curr_thread = (thread_t *)curr_thread->listhead.next;
    } while (list_is_head(&curr_thread->listhead, run_queue));
    switch_to(&tmp->context, &curr_thread->context);
    enable_irq();
}

void kill_zombies()
{
    disable_irq();
    list_head_t *curr;
    list_for_each(curr, run_queue)
    {
        if (((thread_t *)curr)->iszombie)
        {
            list_del_entry(curr);   //delete from run queue
            kfree(((thread_t *)curr)->stack_allocted_ptr);   //free user stack
            kfree(((thread_t *)curr)->kernel_stack_allocted_ptr);    //free kernel stack
            ((thread_t *)curr)->iszombie = 0;
            ((thread_t *)curr)->isused = 0;
        }
    }
    enable_irq();
}

int exec_thread(char *data, unsigned int filesize)
{
    thread_t *tmp = thread_create(data);
    tmp->data = kmalloc(filesize);  
    tmp->datasize = filesize;
    tmp->context.lr = (unsigned long)tmp->data; //set return address

    // copy data
    for (int i = 0; i < filesize; i++)
    {
        tmp->data[i] = data[i];
    }

    curr_thread = tmp;
    add_timer(schedule_timer, 1, "", 0);

    // to EL0
    asm("msr tpidr_el1, %0\n\t" // thread info
        "msr elr_el1, %1\n\t"   // link register
        "msr spsr_el1, xzr\n\t" // Enable interrupt in EL0
        "msr sp_el0, %2\n\t"    // set sp at EL0
        "mov sp, %3\n\t"        // set sp
        "eret\n\t" ::"r"(&tmp->context),
        "r"(tmp->context.lr), "r"(tmp->context.sp), "r"(tmp->kernel_stack_allocted_ptr + KSTACK_SIZE));

    return 0;
}

thread_t *thread_create(void *start)
{
    disable_irq();
    thread_t *tmp;

    if (pid_history > PIDMAX)
        return 0;
    if (!threads[pid_history].isused)
    {
        tmp = &threads[pid_history];
        pid_history += 1;
    }
    else
        return 0;

    tmp->isused = 1;
    tmp->iszombie = 0;
    tmp->signal_inProcess = 0;
    tmp->context.lr = (unsigned long long)start;
    tmp->stack_allocted_ptr = kmalloc(USTACK_SIZE);
    tmp->kernel_stack_allocted_ptr = kmalloc(KSTACK_SIZE);
    tmp->context.sp = (unsigned long long)tmp->stack_allocted_ptr + USTACK_SIZE;
    tmp->context.fp = tmp->context.sp;

    for (int i = 0; i < SIGNAL_MAX; i++)
    {
        tmp->signal_handler[i] = signal_default_handler;
        tmp->sigcount[i] = 0;
    }
    list_add(&tmp->listhead, run_queue);
    enable_irq();
    return tmp;
}

void thread_exit()
{
    disable_irq();
    curr_thread->iszombie = 1;
    enable_irq();
    schedule();
}

void schedule_timer(char *tmp)
{
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq_el0));
    add_timer(schedule_timer, cntfrq_el0 >> 5, "", 1);
}

void foo()
{
    for (int i = 0; i < 10; ++i)
    {
        uart_sendline("Thread id: %d %d\n", curr_thread->pid, i);
        int r = 1000000;
        while (r--)
        {
            asm volatile("nop");
        }
        schedule();
    }
    thread_exit();
}