#include "uart.h"
#include "allocator.h"
#include "timer.h"
#include "cpio.h"
#include "exception.h"
#include "utils.h"
#include "schedule.h"

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
    if (run_queue.head[priority] == NULL) // the head is empty
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

    disable_interrupt();
    cur_task->state = EXIT; // set the task state to EXIT
    enable_interrupt();

    schedule(); // yield the CPU
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
    disable_interrupt();
    for (int i = 0; i < 10; i++)
    {
        task_struct *cur = run_queue.head[i];
        while (cur != NULL && cur->state == EXIT)
        {
            task_struct *del = cur;
            cur = cur->next;

            run_queue_remove(del, i); // remove zombie task

            kfree(del->kstack); // free the space
            kfree(del->ustack);
            kfree(del);
        }
    }
    enable_interrupt();
}

void schedule()
{
    task_struct *cur = get_current_task()->next;
    while (cur != NULL && cur->state == IDLE)
        cur = cur->next;

    if (cur == NULL)
        cur = run_queue.head[0];
    
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
    if (cur->need_sched == 1) // if the need_sched flag of thecurrent task is set, then call schedle()
    {   
        disable_interrupt();
        cur->need_sched = 0;
        enable_interrupt();

        schedule();
    }
}

void do_exec(const char *name, char *const argv[])
{
    for (int i = 0; i < file_num; i++)
    {
        if (my_strcmp(file_arr[i].path_name, name) == 0) // find the file in the init ramdisk
        {
            char *content = file_arr[i].file_content;
            int size = given_size_hex_atoi(file_arr[i].file_header->c_filesize, 8);
            char *target = kmalloc(size);
            char *copy = target;

            while (size--) // move the file content to memory
                *copy++ = *content++;

            task_struct *cur = get_current_task();
            cur->priority = 1;
            // set sp_el0 to user stack, sp_el1 to kernels stack, elr_el1 to the file content
            asm volatile(
                "msr sp_el0, %0\n"
                "msr elr_el1, %1\n"
                "mov x10, 0\n"
                "msr spsr_el1, x10\n"
                "mov sp, %2\n"
                "eret\n"
                :
                : "r"(cur->ustack), "r"(target), "r"(cur->kstack));
        }
    }
}

void exec_fun(void (*func)(void))
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