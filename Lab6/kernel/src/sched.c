#include "sched.h"
#include "irq.h"
#include "memory.h"
#include "mini_uart.h"
#include "slab.h"
#include "timer.h"
#include "utils.h"

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
    // struct task_struct *new_task = alloc_task();
    struct task_struct* new_task =
        (struct task_struct*)((unsigned long)alloc_task());
    if (!new_task)
        return NULL;
    memset(new_task, 0, sizeof(struct task_struct));
    new_task->state = TASK_INIT;
    new_task->pid = nr_tasks++;
    new_task->priority = priority;
    new_task->preempt_count = preempt_count;
    new_task->mm.pgd = pg_dir;
    INIT_LIST_HEAD(&new_task->mm.mmap_list);
    return new_task;
}

void add_task(struct task_struct* task)
{
    disable_irq();
    list_add_tail(&task->list, &running_queue);
    enable_irq();
}

void kill_task(struct task_struct* task)
{
    if (!task)
        return;
    preempt_disable();
    task->state = TASK_STOPPED;
    list_del_init(&task->list);

    list_add(&task->list, &stopped_queue);
    preempt_enable();
    schedule();
}

struct task_struct* find_task(int pid)
{
    struct task_struct* curr;
    list_for_each_entry (curr, &running_queue, list) {
        if (curr->pid == pid)
            return curr;
    }
    return NULL;
}

void wait_task(struct task_struct* task, struct task_struct* wait_for)
{
    if (!task || !wait_for)
        return;
    if (task->state == TASK_WAITING)
        return;
    preempt_disable();
    task->wait_task = wait_for;
    task->state = TASK_WAITING;
    list_del(&task->list);
    list_add_tail(&task->list, &waiting_queue);
    preempt_enable();
}

void exit_process(void)
{
    kill_task(current_task);
}

void delete_task(struct task_struct* task)
{
    if (task->kernel_stack)
        kfree(task->kernel_stack);
    if (task->user_stack)
        kfree(task->user_stack);
    if (task->prog)
        kfree(task->prog);
    if (task->sig_stack)
        kfree(task->sig_stack);
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

void check_waiting(void)
{
    disable_irq();
    struct task_struct *entry, *safe;
    list_for_each_entry_safe (entry, safe, &waiting_queue, list) {
        if (entry->wait_task->flags & PF_WAIT &&
            entry->wait_task->state == TASK_STOPPED) {
            entry->wait_task = NULL;
            list_del(&entry->list);
            entry->state = TASK_RUNNING;
            list_add_tail(&entry->list, &running_queue);
        }
    }
    enable_irq();
}

int sched_init(void)
{
    task_struct =
        kmem_cache_create("task_struct", sizeof(struct task_struct), -1);

    if (!task_struct)
        return 0;

    set_current_task(&init_task);
    init_task.mm.pgd = pg_dir;
    INIT_LIST_HEAD(&init_task.mm.mmap_list);
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
    set_pgd(next->mm.pgd);
    cpu_switch_to(&current_task->cpu_context, &next->cpu_context);
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
