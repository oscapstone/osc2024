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
    SYS_MBOX_CALL,      // doen
    SYS_KILL,           // done
    SYS_SIGNAL_REGISTER,// 8
    SYS_SIGNAL,         // 9
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
    SYS_SIGRETURN,
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
void sys_signal_register(TRAP_FRAME* regs);
void sys_signal(TRAP_FRAME* trap_frame);
void sys_sigreturn(TRAP_FRAME* trap_frame);
void sys_mmap(TRAP_FRAME* trap_frame);
void sys_read(TRAP_FRAME* regs);
void sys_write(TRAP_FRAME* regs);
void sys_mkdir(TRAP_FRAME* regs);
void sys_mount(TRAP_FRAME* regs);
void sys_chdir(TRAP_FRAME* regs);
void sys_lseek64(TRAP_FRAME* regs);
void sys_ioctl(TRAP_FRAME* regs);