#include "fork.h"
#include "arm/sysregs.h"
#include "memory.h"
#include "mini_uart.h"
#include "mm.h"
#include "slab.h"
#include "utils.h"

int copy_process(unsigned long clone_flags, void* fn, void* arg1, void* arg2)
{
    preempt_disable();
    struct task_struct* new_task = create_task(current_task->priority, 1);

    if (!new_task)
        return -1;

    // new_task->kernel_stack = kmalloc(THREAD_STACK_SIZE, 0);
    new_task->kernel_stack = (void*)allocate_kernel_pages(KER_STK_SZ, 0);

    if (!new_task->kernel_stack)
        return -1;

    struct pt_regs* child_regs = task_pt_regs(new_task);

    if (!child_regs)
        return -1;

    memset(child_regs, 0, sizeof(struct pt_regs));

    if (clone_flags & PF_KTHREAD) {
        new_task->cpu_context.x19 = (unsigned long)fn;
        new_task->cpu_context.x20 = (unsigned long)arg1;
        new_task->cpu_context.x21 = (unsigned long)arg2;
    } else {
        struct pt_regs* curr_regs = task_pt_regs(current_task);
        *child_regs = *curr_regs;
        child_regs->regs[0] = 0;
        copy_virt_memory(new_task);


        // new_task->user_stack = kmalloc(THREAD_STACK_SIZE, 0);

        // if (!new_task->user_stack)
        //     return -1;

        // child_regs->sp =
        //     (unsigned long)new_task->user_stack + THREAD_STACK_SIZE;

        // if (current_task->user_stack) {
        //     unsigned long offset = (unsigned long)current_task->user_stack +
        //                            THREAD_STACK_SIZE - curr_regs->sp;
        //     child_regs->sp -= offset;
        //     memcpy((void*)child_regs->sp, (const void*)curr_regs->sp,
        //     offset);
        // }

        // for (int i = 0; i < NR_SIGNAL; i++)
        //     new_task->sig_handler[i] = current_task->sig_handler[i];
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
    return (struct pt_regs*)((unsigned long)task->kernel_stack + KER_STK_SZ -
                             sizeof(struct pt_regs));
}

int move_to_user_mode(unsigned long pc, unsigned long size)
{
    preempt_disable();
    struct pt_regs* regs = task_pt_regs(current_task);
    if (!regs)
        return -1;

    memset(regs, 0, sizeof(struct pt_regs));

    current_task->prog_size = size;



    uart_printf("allocate user pages\n");
    void* target = (void*)allocate_user_pages(current_task, PROG, USR_CODE_ADDR,
                                              current_task->prog_size, 0);
    uart_printf("allocate done\n");

    if (!target)
        return -1;

    regs->pc = USR_CODE_ADDR;
    current_task->prog = target;

    uart_printf("memcpy to user pages\n");
    memcpy(target, (void*)pc, current_task->prog_size);
    uart_printf("memcpy done\n");


    regs->pstate = SPSR_EL0t;

    if (current_task->user_stack)
        goto set_sp;

    uart_printf("allocate user pages\n");
    void* stack = (void*)allocate_user_pages(current_task, STK, USR_STK_ADDR,
                                             USR_STK_SZ, 0);
    uart_printf("allocate done\n");

    if (!stack)
        return -1;


    current_task->user_stack = stack;


    // uart_printf("allocate user pages\n");
    // void* io = (void*)allocate_user_pages(current_task, IO,
    //                                       IO_PM_START_ADDR + VA_START,
    //                                       IO_PM_END_ADDR - IO_PM_START_ADDR,
    //                                       0);
    // uart_printf("allocate done\n");
    //
    map_pages(current_task, IO, IO_PM_START_ADDR, IO_PM_START_ADDR,
              IO_PM_END_ADDR - IO_PM_START_ADDR);
    add_vm_area(current_task, IO, IO_PM_START_ADDR, IO_PM_START_ADDR,
                IO_PM_END_ADDR - IO_PM_START_ADDR);



    // current_task->user_stack =
    // kmalloc(THREAD_STACK_SIZE, 0);

    // if (!current_task->user_stack)
    //     return -1;

set_sp:
    regs->sp = USR_STK_ADDR + USR_STK_SZ;
    //     regs->sp = (unsigned long)current_task->user_stack +
    //     THREAD_STACK_SIZE;
    set_pgd(current_task->mm.pgd);

    preempt_enable();
    return 0;
}
