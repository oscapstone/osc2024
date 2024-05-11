#include "uart.h"
#include "allocator.h"
#include "timer.h"
#include "kernel_task.h"

task_list run_queue;
int task_id = 0;

void task_init()
{
    for (int i = 0; i < 10; i++)
        run_queue.head[i] = NULL;

    // create idle task
    task_create(idle, 0);
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

int task_create(void (*start_routine)(void), int priority)
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

    run_queue_push(new_task, priority);

    return new_task->id;
}

void task_exit()
{
    task_struct *cur_task = get_current_task();
    cur_task->state = EXIT;
    schedule();
}

void idle()
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

            run_queue_remove(del, i);

            kfree(del->kstack);
            kfree(del->ustack);
            kfree(del);
        }
    }
}

void schedule()
{
    task_struct *cur = get_current_task()->next;
    while (cur != NULL && cur->state == IDLE)
    {
        cur = cur->next;
    }

    if (cur == NULL)
        cur = run_queue.head[cur->priority];
    context_switch(cur);
}

void context_switch(struct task_struct *next)
{
    struct task_struct *prev = get_current_task();
    switch_to(prev, next);
}

void check_need_schedule()
{
    struct task_struct *cur = get_current_task();
    if(cur->need_sched == 1)
    {
        cur->need_sched = 0;
        schedule();
    }
}

void foo()
{
    for (int i = 0; i < 10; ++i)
    {
        task_struct *cur_task = get_current_task();

        uart_puts("Thread id: ");
        uart_dec(cur_task->id);
        uart_puts(" ");
        uart_dec(i);
        uart_puts("\n");

        while (cur_task->need_sched == 0)
            ;

        if (cur_task->need_sched == 1)
        {
            cur_task->need_sched = 0;
            schedule();
        }
    }
    task_exit();
}