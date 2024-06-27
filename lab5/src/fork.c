#include "../include/mem_utils.h"
#include "../include/sched.h"
#include "../include/fork.h"
#include "../include/list.h"
#include "../include/entry.h"
#include "../include/mm.h"

int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg, unsigned long stack)
{
    preempt_disable();
    struct task_struct *p;

    p = (struct task_struct *)page_frame_allocate(4);
    if (!p)
        return -1;

    struct pt_regs *childregs = task_pt_regs(p);
    memzero((unsigned long)childregs, sizeof(struct pt_regs));
    memzero((unsigned long)&p->cpu_context, sizeof(struct cpu_context));

    if (clone_flags & PF_KTHREAD) {
        p->cpu_context.x19 = fn;
        p->cpu_context.x20 = arg;
    } else {
        struct pt_regs *cur_regs = task_pt_regs(current);
        *childregs = *cur_regs;
        childregs->regs[0] = 0;
        childregs->sp = stack + PAGE_SIZE;
        p->stack = stack;
    }
    p->flags = clone_flags;
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1;          // disable preemption until schedule_tail
    p->pid = nr_tasks++;

    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)childregs;
    list_add_tail(&p->cpu_context.task_head, &task_head_start);
    preempt_enable();
    return p->pid;
}

int move_to_user_mode(unsigned long pc)
{
    struct pt_regs *regs = task_pt_regs(current);
    memzero((unsigned long)regs, sizeof(*regs));
    regs->pc = pc;
    regs->pstate = PSR_MODE_EL0t;
    unsigned long stack = page_frame_allocate(4);    // allocate new user stack
    if (!stack)
        return -1;
    regs->sp = stack + PAGE_SIZE;
    current->stack = stack;
    return 0;
}

struct pt_regs *task_pt_regs(struct task_struct *tsk)
{
    unsigned long p = (unsigned long)tsk + THREAD_SIZE - sizeof(struct pt_regs);
    return (struct pt_regs *)p;
}