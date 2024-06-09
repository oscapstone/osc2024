#include "sched.h"
#include "slab.h"

int copy_process(void* fn, void* arg)
{
    preempt_disable();
    struct task_struct* new_task = create_task(current_task->priority, 1);

    if (!new_task)
        return -1;

    new_task->counter = new_task->priority;

    new_task->stack = kmalloc(THREAD_STACK_SIZE, 0);

    if (!new_task->stack)
        return -1;

    new_task->state = TASK_RUNNING;

    new_task->cpu_context.x19 = (unsigned long)fn;
    new_task->cpu_context.x20 = (unsigned long)arg;
    new_task->cpu_context.pc = (unsigned long)ret_from_fork;
    new_task->cpu_context.sp =
        (unsigned long)new_task->stack + THREAD_STACK_SIZE;

    add_task(new_task);

    preempt_enable();

    return 0;
}
