#include "delays.h"
#include "sched.h"
#include "exception.h"
#include "exec.h"
#include "timer.h"
#include "demo.h"
#include "uart.h"

struct task_struct task_pool[NR_TASKS];
char kstack_pool[NR_TASKS][KSTACK_SIZE];
char ustack_pool[NR_TASKS][USTACK_SIZE];
int num_running_task = 0;

/* 
// Codes below is the structure used in linux 0.11. But useless in osdi (hierarychy difference)
union task_union {
    struct task_struct task;
    char stack[PAGE_SIZE];
};
static union task_union init_task = {INIT_TASK, };
struct task_struct *current = &(init_task.task);
*/

void task_init()
{
    for (int i = 0; i < NR_TASKS; i++) {
        task_pool[i].state = TASK_STOPPED;
        task_pool[i].task_id = i;
    }

    // TODO: Understand how to deal with the task[0] (kernel). At linux 0.11, it is a special task.
    task_pool[0].state = TASK_RUNNING;
    task_pool[0].priority = 1;
    task_pool[0].counter = 1;

    // I don't have to initialize the tss of task[0], because when it switch to other task, the tss of task[0] will be saved to its own stack.
    // Maybe I should initialize the tss.sp or sp ?
    update_current(&task_pool[0]);
    num_running_task = 1;
}

void context_switch(struct task_struct *next)
{
    struct task_struct *prev = current; // the current task_struct address
    update_current(next);
    switch_to(&prev->tss, &next->tss);
}

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
            printf("Setup task %d, priority %d, counter %d, sp 0x%x, lr 0x%x\n", i, task_pool[i].priority, task_pool[i].counter, task_pool[i].tss.sp, task_pool[i].tss.lr);    
    } else {
        uart_puts("task pool is full, can't create a new task.\n");
        while (1);
    }
    return i;
}

/* Select the task with the highest counter to run. If all tasks' counter is 0, then update all tasks' counter with their priority. */
void schedule()
{
    int i = NR_TASKS, c = -1, next = 0;
    while (1) {
        while (--i) {
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
    // printf("schedule to task %d\n", next);
    context_switch(&task_pool[next]);
}

void idle() {
    while (1) {
        if(num_running_task == 1) {
            break;
        }
        schedule();
        wait_sec(1);
        // wait_msec(100);
    }
    printf("Test finished\n");
    while(1);
}


void sched_init()
{
    /* for OSDI-2020 Lab 4 requirement 1 */
    // privilege_task_create(demo_task1, 10);
    // privilege_task_create(demo_task2, 10);

    /* for OSDI-2020 Lab 4 requirement 2 */
    // privilege_task_create(timer_task1, 3);
    // privilege_task_create(timer_task2, 3);

    /* for OSDI-2020 Lab 4 requirement 3 */
    // privilege_task_create(demo_do_exec1, 5);
    // privilege_task_create(demo_do_exec2, 5);

    /* for OSDI-2020 Lab 4 requirement 4 */
    // privilege_task_create(user_test, 5);
    // core_timer_enable();
    // idle();

    enable_interrupt(); // for requirement 2 of OSDI 2020 Lab4. We enable interrupt here. Because we want timer interrupt at EL1.

    core_timer_enable(); // enable core timer.

    schedule();
}