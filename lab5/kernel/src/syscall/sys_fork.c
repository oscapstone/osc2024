
#include "syscall.h"
#include "proc/task.h"
#include "utils/printf.h"
#include "utils/utils.h"
#include "mm/mm.h"
#include "arm/mmu.h"
#include "peripherals/irq.h"

extern TASK_MANAGER* task_manager;

void sys_fork(TRAP_FRAME* regs) {
    const TASK* task = task_get_current_el1();
    NS_DPRINT("[SYSCALL][TRACE] fork called pid = %d\n", task->pid);
    int parent_pid = task->pid;
    
    char name[20];
    sprintf(name, "%s_child", task->name);

    lock_interrupt();
    TASK* new_task = task_create_user(name, task->flags);
    new_task->priority = task->priority;
    new_task->parent_pid = parent_pid;

    //  copy VMA page table
    mmu_fork_mm(task, new_task);

    task_run(new_task);

    // copy the stacks
    memcpy(task->kernel_stack, new_task->kernel_stack, TASK_STACK_SIZE);
    memcpy(task->user_stack, new_task->user_stack, TASK_STACK_SIZE);

    // record the current context specialy return address to split the child and parent task
    task_asm_store_context(task_get_current_el1());

    // split here
    task = task_get_current_el1();
    NS_DPRINT("[FORK] split pid: %d\n", task->pid);

    if (parent_pid != task->pid) {              // if current process is child process
        //NS_DPRINT("child process fork!\n");
        //NS_DPRINT("child SP: 0x%x, FP: 0x%x\n", task->cpu_regs.sp, task->cpu_regs.fp);

        // Fix: set the return value for child in parent
        // wired regs is parent stack, I have do idea why <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        //regs->regs[0] = 0;
        //NS_DPRINT("child regs->x0 = %d, regs addr:0x%x\n", regs->regs[0], regs);
        return;
    }

    // parent process continue

    // copy the register status, the parent process will continue here because of disabling interrupt when in trap at default
    U64 tmp_pgd = new_task->cpu_regs.pgd;
    memcpy(&task->cpu_regs, &new_task->cpu_regs, sizeof(CPU_REGS));
    new_task->cpu_regs.pgd = MMU_VIRT_TO_PHYS(tmp_pgd);
    NS_DPRINT("new task GPD addr: 0x%x\n", new_task->cpu_regs.pgd);
    new_task->cpu_regs.fp += (U64)new_task->kernel_stack - (U64)task->kernel_stack;
    new_task->cpu_regs.sp += (U64)new_task->kernel_stack - (U64)task->kernel_stack;
    NS_DPRINT("[FORK] split pid: %d, parent_pid: %d, new_task_pid: %d\n", task->pid, parent_pid, new_task->pid);
    NS_DPRINT("new_task SP: 0x%x, FP: 0x%x\n", new_task->cpu_regs.sp, new_task->cpu_regs.fp);

    TRAP_FRAME* child_trapFrame = (TRAP_FRAME*)((UPTR)regs + (U64)new_task->kernel_stack - (U64)task->kernel_stack);
    child_trapFrame->regs[0] = 0;   // set the child x0 = 0 for return value


    unlock_interrupt();


    regs->regs[0] = new_task->pid;
    return;
}
