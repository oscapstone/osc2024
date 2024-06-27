#include "../include/mem_utils.h"
#include "../include/list.h"
#include "../include/mini_uart.h"
#include "../include/timer.h"
#include "../include/timer_utils.h"
#include "../include/peripherals/mini_uart.h"
#include "../include/task_queue.h"
#include "../include/sched.h"
#include "../include/exception.h"
#include <stdint.h>

#define UNUSED(x) UNUSED_##x __attribute__((__unused__))

/* global variables for scheduler */
static struct task_struct init_task = INIT_TASK(init_task);
struct task_struct *current = &(init_task);
struct list_head task_head_start = {&(init_task.cpu_context.task_head), &(init_task.cpu_context.task_head)};
uint32_t nr_tasks = 1;

/* scheduler function */
void show_task_head()
{
    printf("The address of task_head: %8x\n", current);
    printf("The address that task_head_start.next: %8x\n", task_head_start.next);
    printf("The address that task_head_start.prev: %8x\n", task_head_start.prev);    
}

void kill_process(int pid)
{
    int find = 0;
    for (struct list_head *curr = task_head_start.next; curr != &task_head_start; curr = curr->next) {
        if (((struct task_struct *)curr)->pid == pid) {
            ((struct task_struct *)curr)->state = TASK_ZOMBIE;
            find = 1;
            break;
        }
    }
    if (find)
        kill_zombies();
    else
        printf("No process id: %d exist\n", pid);
}

void exit_process(void)
{
    /* print out the pid to check the if it access the correct process */
    printf("Now pid: %d\n", current->pid);

    /* disable preemption */
    preempt_disable();

    /* modify the process state */
    current->state = TASK_ZOMBIE;

    /* free current->stack */
    if (current->stack)
        page_frame_free((char *)current->stack);

    /* enable preemption */
    preempt_enable();

    /* kill zombies */
    kill_zombies();

    /* After enable preemption, it "must" call schedule or it would go to strange address */
    schedule();
}

void kill_zombies(void)
{
    /* first, disable preemption while kill zombies */
    preempt_disable();

    /* then, traverse the task list to remove stopped task */
    struct list_head *curr, *safety;
    for (curr = task_head_start.next, safety = curr->next; curr != &task_head_start; curr = safety, safety = safety->next) {
        if (((struct task_struct *)curr)->state == TASK_ZOMBIE) {
            printf("Delete the process\n");
            list_del(curr);
        }
    }

    /* after remove all stopped task, enable preemption */
    preempt_enable();
}

void idle(void)
{
    while (1) {
        printf("In main thread\n");
        kill_zombies();
        schedule();
    }
}

void preempt_disable(void)
{
    current->preempt_count++;
}

void preempt_enable(void)
{
    current->preempt_count--;
}

/* static function of scheduler */
static void _schedule(void)
{
    preempt_disable();
    int c;
    struct task_struct *next;
    while (1) {
        c = -1;
        /* find the task which has biggest counter */
        for (struct list_head *p = task_head_start.next; p != &task_head_start; p = p->next) {
            struct task_struct *tmp = (struct task_struct *)p;
            if (tmp && tmp->state == TASK_RUNNING && tmp->counter > c) {
                c = tmp->counter;
                next = tmp;
            } 
        }
        if (c > 0)
            break;
        /* if there is no task has counter bigger than zero, than rasie the counter of all tasks */
        for (struct list_head *p = task_head_start.next; p != &task_head_start; p = p->next) {
            struct task_struct *tmp = (struct task_struct *)p;
            if (tmp)
                tmp->counter = (tmp->counter >> 1) + tmp->priority;
        }
    }
    switch_to(next);
    preempt_enable();
}

void schedule(void)
{
    current->counter = 0;
    _schedule();
}

void switch_to(struct task_struct *next)
{
    if (current == next)
        return;
    struct task_struct *prev = current;
    current = next;
    cpu_switch_to(&prev->cpu_context, &next->cpu_context);
}

void schedule_tail(void) {
	preempt_enable();
}

void timer_tick(char * UNUSED(msg))
{
    --current->counter;
    if (current->counter > 0 || current->preempt_count > 0) {
        return;
    }
    current->counter = 0;
    enable_interrupt();
    _schedule();
    disable_interrupt();
}