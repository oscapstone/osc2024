#pragma once

#include "io/trapFrame.h"

// define SYS_CALL = void(TRAP_FRAME*)
typedef void (*SYS_CALL) (TRAP_FRAME*);

enum {
    SYS_GETPID,
    SYS_UART_READ,
    SYS_UART_WRITE,
    SYS_EXEC,
    SYS_FORK,
    SYS_EXIT,
    SYS_MBOX_CALL,
    SYS_KILL,
    NR_SYSCALLS
};

void syscall_handler(TRAP_FRAME* trap_frame);


// syscalls
void sys_getpid(TRAP_FRAME *regs);
void sys_uartread(TRAP_FRAME *regs);
void sys_uartwrite(TRAP_FRAME* regs);
void sys_exec(TRAP_FRAME* regs);
void sys_fork(TRAP_FRAME* regs);
void sys_exit(TRAP_FRAME* regs);
void sys_mbox_call(TRAP_FRAME* regs);
void sys_kill(TRAP_FRAME* regs);