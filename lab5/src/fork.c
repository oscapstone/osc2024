#include "../include/mem_utils.h"
#include "../include/sched.h"
#include "../include/fork.h"
#include "../include/list.h"

int copy_process(unsigned long fn, unsigned long arg)
{
    preempt_disable();
    struct task_struct *p;

    p = (struct task_struct *)page_frame_allocate(4);
    if (!p)
        return 1;
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1;          // disable preemption until schedule_tail
    p->pid = nr_tasks++;

    p->cpu_context.x19 = fn;
    p->cpu_context.x20 = arg;
    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;
    list_add_tail(&p->cpu_context.task_head, &task_head_start);
    preempt_enable();
    return 0;
}