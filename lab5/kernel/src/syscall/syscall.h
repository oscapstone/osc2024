#pragma once

#include "io/trapFrame.h"

// define SYS_CALL = void(TRAP_FRAME*)
typedef void (*SYS_CALL) (TRAP_FRAME*);

enum {
    SYS_GETPID,         // done
    SYS_UART_READ,      // done
    SYS_UART_WRITE,     // done
    SYS_EXEC,           
    SYS_FORK,           // done
    SYS_EXIT,           // done
    SYS_MBOX_CALL,
    SYS_KILL,           // done
    SYS_SIGNAL,         // 8
    SYS_SIGKILL,        // 9
    SYS_MMAP,           // 10
    SYS_OPEN,
    SYS_CLOSE,
    SYS_WRITE,
    SYS_READ,
    SYS_MKDIR,
    SYS_MOUNT,
    SYS_CHDIR,
    SYS_LSEEK64,
    SYS_IOCTL,
    SYS_SYNC,
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
void sys_open(TRAP_FRAME* regs);
void sys_close(TRAP_FRAME* regs);