
#include "syscall.h"

void sys_exec(TRAP_FRAME* regs) {
    char* name = (char*) regs->regs[0];
}
