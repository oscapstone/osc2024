#include "sched.h"
#include "exception.h"
#include "memory.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

struct list_head *runqueue;
thread_t threads[PIDMAX + 1];
thread_t *current = NULL;

int pid_history = 0;
int thread_sched_init_finished = 0;

void thread_schedule_init()
{
    lock();

    runqueue = s_allocator(sizeof(struct list_head));
    INIT_LIST_HEAD(runqueue);

    // init pids
    for (int i = 0; i <= PIDMAX; i++) {
        threads[i].used = 0;
        threads[i].zombie = 0;
        threads[i].pid = i;
    }

    current = thread_create(idle, IDLE_PRIORITY);
    thread_create(shell, SHELL_PRIORITY);

    // explain : tpidr_el1 is a register that stores the current thread pointer
    asm volatile("msr tpidr_el1, %0" ::"r"(current + sizeof(struct list_head))); // set current thread to tpidr_el1
    thread_sched_init_finished = 1;

    unsigned long long cntpct, cntfrq;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));
    add_timer(schedule_timer, "", cntpct + cntfrq); // 1 second

    unlock();
}

void idle()
{
    while (1) {
        kill_zombies();
        asm volatile("msr DAIFClr, 0xf");
        // uart_printf("Idle thread is running\n");
        schedule();
    }
}

thread_t *thread_create(void *start, int priority)
{
    lock();
    thread_t *r;
    if (pid_history > PIDMAX)
        return 0;
    if (!threads[pid_history].used) {
        r = &threads[pid_history];
        r->pid = pid_history++;
    }
    else
        return 0;

    r->zombie = 0;
    r->priority = priority;
    r->used = 1;
    r->stack_ptr = (char *)kmalloc(USTACK_SIZE);
    r->kstack_ptr = (char *)kmalloc(KSTACK_SIZE);
    r->cpu_context.lr = (unsigned long)start;
    r->cpu_context.sp = (unsigned long)(r->stack_ptr + USTACK_SIZE);
    r->cpu_context.fp = (unsigned long)(r->stack_ptr + USTACK_SIZE);

    add_task_thread(r);
    unlock();

    return r;
}

void add_task_thread(thread_t *t)
{
    lock();
    struct list_head *pos;
    int flag = 1;
    list_for_each(pos, runqueue)
    {
        if (((thread_t *)pos)->priority > current->priority) {
            list_add(&t->listhead, pos->prev);
            flag = 0;
            break;
        }
    }

    if (flag) {
        list_add_tail(&t->listhead, runqueue);
    }

    unlock();
}

void schedule()
{
    list_del(&current->listhead);
    add_task_thread(current);
    current = (thread_t *)runqueue->next;
    while (current->zombie) {
        current = (thread_t *)current->listhead.next;
    }
    switch_to(get_current(), &current->cpu_context);
}

void kill_zombies()
{
    lock();
    struct list_head *cur;
    list_for_each(cur, runqueue)
    {
        // uart_printf("Pid %d\n", ((thread_t *)cur)->pid);
        if (((thread_t *)cur)->zombie) {
            list_del(cur);
            kfree(((thread_t *)cur)->stack_ptr);
            kfree(((thread_t *)cur)->kstack_ptr);
            ((thread_t *)cur)->used = 0;
            ((thread_t *)cur)->zombie = 0;
            uart_printf("Zombie found with pid: %d.\n", ((thread_t *)cur)->pid);
        }
    }
    unlock();
}

void schedule_timer(char *notuse)
{
    // uart_printf("Schedule timer\n");
    unsigned long long cntpct, cntfrq;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));
    add_timer(schedule_timer, "", (cntfrq >> 5) + cntpct); // 1/32 second to schedule
}

void foo()
{
    for (int i = 0; i < 5; ++i) {
        uart_printf("Thread id: %d %d\n", current->pid, i);
        _delay(1000000);
        schedule();
    }
    thread_exit();
}

void _delay(unsigned int count)
{
    while (count--)
        asm volatile("nop");
}

void thread_exit()
{
    // thread cannot deallocate the stack while still using it, wait for someone to recycle it.
    // In this lab, idle thread handles this task, instead of parent thread.
    lock();
    uart_printf("Thread %d exit\n", current->pid);
    current->zombie = 1;
    unlock();
    schedule();
}

int exec_thread(char *data, unsigned int datasize)
{
    thread_t *t = thread_create(run_user_process, NORMAL_PRIORITY);

    lock();
    t->data = kmalloc(datasize);
    t->datasize = datasize;
    for (int i = 0; i < datasize; i++)
        t->data[i] = data[i];
    current = t;
    unlock();

    switch_to(get_current(), &t->cpu_context);

    return 0;
}

void run_user_process()
{
    asm("msr tpidr_el1, %0\n\t"
        "msr elr_el1, %1\n\t"
        "msr spsr_el1, xzr\n\t"
        "msr sp_el0, %2\n\t"
        "mov sp, %3\n\t"
        "eret\n\t" ::"r"(&current->cpu_context),
        "r"((unsigned long)current->data), "r"(current->cpu_context.sp), "r"(current->kstack_ptr + KSTACK_SIZE));
}