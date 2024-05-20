
#include "syscall.h"
#include "proc/task.h"
#include "utils/printf.h"

void sys_exit(TRAP_FRAME* regs) {
    NS_DPRINT("[SYSCALL][EXIT] task exited with status: %d\n", regs->regs[0]);
    task_exit();
}
