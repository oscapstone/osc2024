#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <lib/stdlib.h>
#include <lib/string.h>

extern void ret_from_fork();

int copy_process(unsigned long clone_flags, unsigned long func,
                 unsigned long arg, unsigned long stack) {
    print_string("[copy_process] begin\n");
    preempt_disable();  // copy process is not preemptable

    print_string("[copy_process] kmalloc stack: ");
    task_struct_t *new_task = (task_struct_t *)kmalloc(STACK_SIZE);
    // struct task_struct *new_task =
        // (task_struct_t *)kmalloc(sizeof(task_struct_t));

    print_string("[copy_process] new_task: ");
    print_h((uint64_t)new_task);

    if (!new_task) {
        print_string("allocate new task failed\n");
        return -1;
    }

    struct pt_regs *regs = task_pt_regs(new_task);
    memset(regs, 0, sizeof(struct pt_regs));
    memset(&new_task->context, 0, sizeof(struct cpu_context));

    if (clone_flags & PF_KTHREAD) {
        new_task->context.x19 = func;
        new_task->context.x20 = arg;
    } else {
        // copy current regs to new process and init return value(regs[0])
        struct pt_regs *current_regs = task_pt_regs(get_current());
        memcpy((void *)regs, (void *)current_regs, sizeof(struct pt_regs));
        regs->regs[0] = 0;  // return value of child process
        unsigned long sp_offset = get_current()->stack + STACK_SIZE - current_regs->sp;
        regs->sp = stack + STACK_SIZE - sp_offset;
        memcpy((void *)regs->sp, (void *)current_regs->sp, sp_offset);
        new_task->stack = stack;
    }

    new_task->flags = clone_flags;

    new_task->priority = 1;
    new_task->state = TASK_RUNNING;
    new_task->counter = new_task->priority;
    new_task->preempt_count = 1;  // disable preempt while creating process

    // 设置任务的程序计数器和堆栈指针
    new_task->context.pc = (unsigned long)ret_from_fork;
    new_task->context.sp = (unsigned long)regs;

    unsigned long pid = get_new_pid();
    new_task->pid = pid;

    // print_string("[copy_process - enqueue_run_queue] pid: ");
    // print_d(pid);
    // print_string(", ");
    // print_h((uint64_t)new_task);
    // print_string("\n");
    enqueue_run_queue(new_task);

    preempt_enable();

    return pid;
}

int move_to_user_mode(unsigned long pc) {
    struct pt_regs *regs =
        task_pt_regs(get_current());  // get current process state

    regs->pc =
        pc;  // point to the function that need to be executed in user mode
    regs->pstate = PSR_MODE_EL0t;  // EL0t is user state

    unsigned long stack =
        (unsigned long)kmalloc(STACK_SIZE);  // allocate new user stack
    memset((void *)stack, 0, STACK_SIZE);

    if (!stack) {  // if stack allocation failed
        print_string("allocate new stack failed\n");
        return -1;
    }

    // allocate stack for user process
    // set sp to the top of the stack
    regs->sp = stack + STACK_SIZE;
    get_current()->stack = stack;
    return 0;
}

// calculate the location of pt_regs in the task_struct
// task_struct is allocated at the end of the stack
// pt_regs is allocated at the top of the task_struct
struct pt_regs *task_pt_regs(task_struct_t *task) {
    unsigned long p = (unsigned long)task + STACK_SIZE - sizeof(struct pt_regs);
    return (struct pt_regs *)p;
}
