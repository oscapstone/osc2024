
#include "syscall.h"
#include "proc/task.h"

void sys_kill(TRAP_FRAME* regs) {
    task_kill(regs->regs[0], -2);
}
