#include "delays.h"
#include "sched.h"
#include "exception.h"
#include "exec.h"
#include "timer.h"
#include "demo.h"
#include "uart.h"
#include "shell.h"
#include "kernel.h"
#include "signal.h"
#include "syscall.h"
#include "mm.h"

struct task_struct task_pool[NR_TASKS];
char kstack_pool[NR_TASKS][KSTACK_SIZE] __attribute__((aligned(16)));
char ustack_pool[NR_TASKS][USTACK_SIZE] __attribute__((aligned(16)));
int num_running_task = 0;

/* Initialize the task_struct and make kernel be task 0. */
void task_init()
{
    for (int i = 0; i < NR_TASKS; i++) {
        task_pool[i].state = TASK_STOPPED;
        task_pool[i].task_id = i;
        task_pool[i].pending = 0;
        task_pool[i].process_sig = 0;
        task_pool[i].sighand = default_sighandler;
        task_pool[i].tss.pgd = (uint64_t) 0;
    }

    // TODO: Understand how to deal with the task[0] (kernel). At linux 0.11, it is a special task.
    task_pool[0].state = TASK_RUNNING;
    task_pool[0].priority = 1;
    task_pool[0].counter = 1;

    // I don't have to initialize the tss of task[0], because when it switch to other task, the tss of task[0] will be saved to its own stack.
    // TODO: check whether we can fork() from task[0].
    update_current(&task_pool[0]);
    num_running_task = 1;
    task_pool[0].tss.lr = (uint64_t) do_user_image;
    task_pool[0].tss.sp = (uint64_t) &kstack_pool[0][KSTACK_TOP];
    task_pool[0].tss.fp = (uint64_t) &kstack_pool[0][KSTACK_TOP];
}

/* Do context switch. */
void context_switch(struct task_struct *next)
{
    struct task_struct *prev = current; // the current task_struct address

#ifdef DEBUG_MULTITASKING
    printf("[context_switch] Switch from task %d to task %d\n", prev->task_id, next->task_id);
#endif
    update_current(next);
    switch_to(&prev->tss, &next->tss);

    /* Before the process returns to user mode, check the pending signals. */
    if (current->pending != 0) {
        /* Allocate signal stack for the task. */
        void *sig_stack = (void *) kmalloc(USTACK_SIZE);

        current->process_sig = current->pending;
        current->pending = 0;
        current->tss.sp_backup = current->tss.sp;

        asm volatile("msr spsr_el1, xzr");
        asm volatile("msr elr_el1, %0" ::"r" (current->sighand.action[current->process_sig]));
        asm volatile("msr sp_el0, %0" ::"r" (sig_stack + USTACK_SIZE - 16));
        asm volatile("mov lr, %0" ::"r" (sigreturn));
        asm volatile("eret");
    }

    if (current->state == TASK_STOPPED)
        schedule();
}

/* Find empty task_struct. Return task id. */
int find_empty_task()
{
    int i;
    for (i = 1; i < NR_TASKS; i++)
        if (task_pool[i].state == TASK_STOPPED)
            break;
    if (i != NR_TASKS)
        num_running_task++;
    return i;
}

/* Create new task and setup its task_struct. Return task id. */
int privilege_task_create(void (*func)(), long priority)
{
    int i = find_empty_task();

    if (i != NR_TASKS) {
            task_pool[i].state = TASK_RUNNING;
            task_pool[i].priority = priority;
            task_pool[i].counter = priority;
            task_pool[i].tss.lr = (uint64_t) func;
            task_pool[i].tss.sp = (uint64_t) &kstack_pool[i][KSTACK_TOP];
            task_pool[i].tss.fp = (uint64_t) &kstack_pool[i][KSTACK_TOP];
#ifdef DEBUG_MULTITASKING
            printf("[privilege_task_create] Setup task %d, priority %d, counter %d, sp 0x%x, lr 0x%x\n", i, task_pool[i].priority, task_pool[i].counter, task_pool[i].tss.sp, task_pool[i].tss.lr);    
#endif
    } else {
        uart_puts("[privilege_task_create] task pool is full, can't create a new task.\n");
        while (1);
    }
    return i;
}

/* Select the task with the highest counter to run. If all tasks' counter is 0, then update all tasks' counter with their priority. */
void schedule(void)
{
#ifdef DEBUG_MULTITASKING
    printf("[schedule] current task %d\n", current->task_id);
#endif
    int i = NR_TASKS, c = -1, next = 0;
    while (1) {
        while ((--i) >= 0) { // With `while (--i)`, the task 0 won't be selected.
            if (task_pool[i].state == TASK_RUNNING && task_pool[i].counter > c) {
                c = task_pool[i].counter;
                next = i;
            }
        }
        if (c)
            break;
        for (i = 0; i < NR_TASKS; i++) // Reset the counter by tasks' priority.
            if (task_pool[i].state == TASK_RUNNING)
                task_pool[i].counter = (task_pool[i].counter >> 1) + task_pool[i].priority;
    }
    if (next != current->task_id)
        context_switch(&task_pool[next]);
}

/* Init task 0, enable core timer, interrupt. Then call schedule(). */
void sched_init()
{
    task_init();

    /* We enable interrupt here. Because we want timer interrupt at EL1. */
    enable_interrupt();
    core_timer_enable();

    /* Start task 0 */
    sig_restore_context(&current->tss);
}