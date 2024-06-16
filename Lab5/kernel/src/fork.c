#include "fork.h"
#include "arm/sysregs.h"
#include "memory.h"
#include "slab.h"

int copy_process(unsigned long clone_flags, void* fn, void* arg)
{
    preempt_disable();
    struct task_struct* new_task = create_task(current_task->priority, 1);

    if (!new_task)
        return -1;

    new_task->kernel_stack = kmalloc(THREAD_STACK_SIZE, 0);

    if (!new_task->kernel_stack)
        return -1;

    struct pt_regs* child_regs = task_pt_regs(new_task);

    if (!child_regs)
        return -1;

    memset(child_regs, 0, sizeof(struct pt_regs));

    if (clone_flags & PF_KTHREAD) {
        new_task->cpu_context.x19 = (unsigned long)fn;
        new_task->cpu_context.x20 = (unsigned long)arg;
    } else {
        struct pt_regs* curr_regs = task_pt_regs(current_task);
        *child_regs = *curr_regs;
        child_regs->regs[0] = 0;
        new_task->user_stack = kmalloc(THREAD_STACK_SIZE, 0);

        if (!new_task->user_stack)
            return -1;

        child_regs->sp =
            (unsigned long)new_task->user_stack + THREAD_STACK_SIZE;

        if (current_task->user_stack) {
            unsigned long offset = (unsigned long)current_task->user_stack +
                                   THREAD_STACK_SIZE - curr_regs->sp;
            child_regs->sp -= offset;
            memcpy((void*)child_regs->sp, (const void*)curr_regs->sp, offset);
        }

        for (int i = 0; i < NR_SIGNAL; i++)
            new_task->sig_handler[i] = current_task->sig_handler[i];
    }

    if (clone_flags & PF_WAIT)
        wait_task(current_task, new_task);

    new_task->flags = clone_flags;
    new_task->counter = new_task->priority;
    new_task->state = TASK_RUNNING;

    new_task->cpu_context.pc = (unsigned long)ret_from_fork;
    new_task->cpu_context.sp = (unsigned long)child_regs;

    add_task(new_task);

    preempt_enable();

    return new_task->pid;
}


struct pt_regs* task_pt_regs(struct task_struct* task)
{
    if (!task->kernel_stack)
        return NULL;
    return (struct pt_regs*)((unsigned long)task->kernel_stack +
                             THREAD_STACK_SIZE - sizeof(struct pt_regs));
}

int move_to_user_mode(unsigned long pc)
{
    struct pt_regs* regs = task_pt_regs(current_task);
    if (!regs)
        return -1;
    memset(regs, 0, sizeof(struct pt_regs));
    regs->pc = pc;
    regs->pstate = SPSR_EL0t;

    current_task->prog = (void*)pc;

    if (current_task->user_stack)
        goto set_sp;

    current_task->user_stack = kmalloc(THREAD_STACK_SIZE, 0);

    if (!current_task->user_stack)
        return -1;

set_sp:
    regs->sp = (unsigned long)current_task->user_stack + THREAD_STACK_SIZE;

    return 0;
}
