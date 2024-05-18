
#include "syscall.h"
#include "proc/task.h"
#include "utils/printf.h"
#include "utils/utils.h"
#include "mm/mm.h"
#include "arm/mmu.h"

extern TASK_MANAGER* task_manager;

void sys_fork(TRAP_FRAME* regs) {
    const TASK* task = task_get_current_el1();
    NS_DPRINT("[SYSCALL][TRACE] fork called pid = %d\n", task->pid);
    int parent_pid = task->pid;
    
    char name[20];
    sprintf(name, "%s_child", task->name);
    TASK* new_task = task_create(name, task->flags);
    new_task->priority = task->priority;
    new_task->parent_pid = parent_pid;

    // TODO: copy VMA page table
    mmu_fork_mm(task, new_task);

    task_run(new_task);

    // record the current context specialy return address to split the child and parent task
    task_asm_store_context(task);

    // split here
    task = task_get_current_el1();

    if (parent_pid != task->pid) {              // if current process is child process
        regs->regs[0] = 0;
        return;
    }

    // parent process continue
    // copy the kernel stack
    memcpy(task->kernel_stack, new_task->kernel_stack, TASK_STACK_SIZE);

    // copy the register status, the parent process will continue here because of disabling interrupt when in trap at default
    memcpy(&task->cpu_regs, &new_task->cpu_regs, sizeof(CPU_REGS));
    new_task->cpu_regs.fp += (U64)new_task->kernel_stack - (U64)task->kernel_stack;
    new_task->cpu_regs.sp += (U64)new_task->kernel_stack - (U64)task->kernel_stack;

    regs->regs[0] = new_task->pid;
    return;
}
