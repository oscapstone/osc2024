#include "uart.h"
#include "allocator.h"
#include "timer.h"
#include "schedule.h"
#include "exception.h"

task_list run_queue;
int task_id = 0;

void sched_init()
{
    for (int i = 0; i < 10; i++)
        run_queue.head[i] = NULL;

    // create idle task
    task_create(zombie_reaper, 0);
    asm volatile("msr tpidr_el1, %0" : : "r"(run_queue.head[0]));
}

void run_queue_push(task_struct *new_task, int priority)
{
    if (run_queue.head[priority] == NULL)
        run_queue.head[priority] = new_task;
    else
    {
        new_task->next = run_queue.head[priority];
        run_queue.head[priority]->prev = new_task;
        run_queue.head[priority] = new_task;
    }
}

void run_queue_remove(task_struct *remove_task, int priority)
{
    if (run_queue.head[priority] == remove_task) // the remove task is the front of the list
    {
        run_queue.head[priority] = run_queue.head[priority]->next;
        if (run_queue.head[priority] != NULL)
            run_queue.head[priority]->prev = NULL;
    }
    else
    {
        if (remove_task->next != NULL)
            remove_task->next->prev = remove_task->prev;

        if (remove_task->prev != NULL)
            remove_task->prev->next = remove_task->next;
    }
}

task_struct *task_create(void (*start_routine)(void), int priority)
{
    // create new task and initialize it.
    task_struct *new_task = kmalloc(sizeof(task_struct));
    new_task->id = task_id++;
    new_task->priority = priority;
    new_task->kstack = (void *)((char *)kmalloc(4096 * 2) + 4096 * 2); // malloc space for kernel stack
    new_task->ustack = (void *)((char *)kmalloc(4096 * 2) + 4096 * 2); // malloc space for user stack
    new_task->cpu_context.sp = (unsigned long long)(new_task->kstack); // set stack pointer
    new_task->cpu_context.fp = (unsigned long long)(new_task->kstack); // set frame pointer
    new_task->cpu_context.lr = (unsigned long long)start_routine;      // set linker register
    new_task->state = RUNNING;
    new_task->need_sched = 0;
    new_task->prev = NULL;
    new_task->next = NULL;

    disable_interrupt();
    run_queue_push(new_task, priority);
    enable_interrupt();

    return new_task;
}

void task_exit()
{
    task_struct *cur_task = get_current_task();
    cur_task->state = EXIT;
    schedule();
}

void zombie_reaper()
{
    while (1)
    {
        kill_zombies();
        schedule();
    }
}

void kill_zombies()
{
    for (int i = 0; i < 10; i++)
    {
        task_struct *cur = run_queue.head[i];
        while (cur != NULL && cur->state == EXIT)
        {
            task_struct *del = cur;
            cur = cur->next;

            disable_interrupt();
            run_queue_remove(del, i);
            enable_interrupt();

            kfree(del->kstack);
            kfree(del->ustack);
            kfree(del);
        }
    }
}

void schedule()
{
    task_struct *cur = get_current_task()->next;
    /*uart_puts("cur task ");
    uart_dec(get_current_task()->id);
    uart_puts("\n");*/

    while (cur != NULL && cur->state == IDLE)
        cur = cur->next;

    if (cur == NULL)
        cur = run_queue.head[0];

    /*uart_puts("schuedle to task ");
    uart_dec(cur->id);
    uart_puts("\n");*/
    context_switch(cur);
}

void context_switch(struct task_struct *next)
{
    struct task_struct *prev = get_current_task();
    if (prev != next)
    {
        switch_to(prev, next);
    }
}

void check_need_schedule()
{
    struct task_struct *cur = get_current_task();
    if (cur->need_sched == 1)
    {
        cur->need_sched = 0;
        schedule();
    }
}

void do_exec(void (*func)(void))
{
    task_struct *cur = get_current_task();
    cur->priority = 1;
    asm volatile(
        "msr sp_el0, %0\n"
        "msr elr_el1, %1\n"
        "mov x10, 0\n"
        "msr spsr_el1, x10\n"
        "mov sp, %2\n"
        "eret\n"
        :
        : "r"(cur->ustack), "r"(func), "r"(cur->kstack));
}