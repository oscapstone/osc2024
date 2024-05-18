
#include "syscall.h"
#include "proc/task.h"

void sys_exit(TRAP_FRAME* regs) {
    task_exit();
}
