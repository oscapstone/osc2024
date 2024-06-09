#include "sched.h"
#include "irq.h"
#include "memory.h"
#include "mini_uart.h"
#include "slab.h"
#include "timer.h"

#define alloc_task()   (struct task_struct*)kmem_cache_alloc(task_struct, 0)
#define free_task(ptr) kmem_cache_free(task_struct, (ptr))

static struct task_struct init_task = INIT_TASK;
pid_t nr_tasks = 1;

static struct kmem_cache* task_struct;

LIST_HEAD(running_queue);
LIST_HEAD(waiting_queue);
LIST_HEAD(stopped_queue);


struct task_struct* create_task(long priority, long preempt_count)
{
    struct task_struct* new_task = alloc_task();
    if (!new_task)
        return NULL;
    mem_set(new_task, 0, sizeof(struct task_struct));
    new_task->state = TASK_INIT;
    new_task->pid = nr_tasks++;
    new_task->priority = priority;
    new_task->preempt_count = preempt_count;
    return new_task;
}

void add_task(struct task_struct* task)
{
    disable_irq();
    list_add_tail(&task->list, &running_queue);
    enable_irq();
}

void kill_task(struct task_struct* task, int status)
{
    disable_irq();
    task->state = TASK_STOPPED;
    list_del_init(&task->list);

    task->exit_code = status;
    list_add(&task->list, &stopped_queue);
    nr_tasks--;
    enable_irq();
    schedule();
}

void delete_task(struct task_struct* task)
{
    if (task->stack)
        kfree(task->stack);
    free_task(task);
}

void kill_zombies(void)
{
    disable_irq();
    struct task_struct *entry, *safe;
    list_for_each_entry_safe (entry, safe, &stopped_queue, list) {
        list_del_init(&entry->list);
        delete_task(entry);
    }
    enable_irq();
}

int sched_init(void)
{
    task_struct =
        kmem_cache_create("task_struct", sizeof(struct task_struct), -1);

    if (!task_struct)
        return 0;

    set_current_context(&init_task.cpu_context);
    add_task(&init_task);
    add_timer(timer_tick, NULL, SCHED_CYCLE, true);

    return 1;
}

void preempt_disable(void)
{
    current_task->preempt_count++;
}

void preempt_enable(void)
{
    current_task->preempt_count--;
}

void _schedule(void)
{
    preempt_disable();
    int c;
    struct task_struct *p, *next;
    while (1) {
        c = -1;
        next = NULL;
        list_for_each_entry (p, &running_queue, list) {
            if (p && p->state == TASK_RUNNING && p->counter > c) {
                c = p->counter;
                next = p;
            }
        }

        if (c)
            break;

        list_for_each_entry (p, &running_queue, list) {
            p->counter = (p->counter >> 1) + p->priority;
        }
    }

    if (next)
        switch_to(next);
    preempt_enable();
}

void schedule(void)
{
    current_task->counter = 0;
    _schedule();
}

void switch_to(struct task_struct* next)
{
    if (current_task == next)
        return;
    cpu_switch_to(current_context, &next->cpu_context);
}

void schedule_tail(void)
{
    preempt_enable();
}

void timer_tick(char* UNUSED(msg))
{
    // uart_printf("timer_tick\n");
    --current_task->counter;
    if (current_task->counter > 0 || current_task->preempt_count > 0)
        return;
    current_task->counter = 0;
    enable_irq();
    _schedule();
    disable_irq();
    // uart_printf("timer_tick end\n");
}
