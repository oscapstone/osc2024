
#include "syscall.h"
#include "proc/task.h"
#include "utils/printf.h"

void sys_getpid(TRAP_FRAME *regs) {
    NS_DPRINT("[SYSCALL][DEBUG] getpid() called. pid = %d\n", task_get_current_el1()->pid);
    regs->regs[0] = task_get_current_el1()->pid;
    return;
}

