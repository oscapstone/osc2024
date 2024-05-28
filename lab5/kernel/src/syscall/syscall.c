

#include "base.h"

#include "syscall.h"
#include "utils/printf.h"
#include "peripherals/irq.h"

void sys_none(TRAP_FRAME* tf);

SYS_CALL syscall_table[NR_SYSCALLS] = {
    [SYS_GETPID] = &sys_getpid,                 // 0
    [SYS_UART_READ] = &sys_uartread,
    [SYS_UART_WRITE] = &sys_uartwrite,
    [SYS_EXEC] = &sys_exec,
    [SYS_FORK] = &sys_fork,
    [SYS_EXIT] = &sys_exit,
    [SYS_MBOX_CALL] = &sys_mbox_call,
    [SYS_KILL] = &sys_kill,
    [SYS_SIGNAL] = &sys_none,
    [SYS_SIGKILL] = &sys_none,
    [SYS_MMAP] = &sys_none,
    [SYS_OPEN] = &sys_none,
    [SYS_CLOSE] = &sys_none,
    [SYS_WRITE] = &sys_none,
    [SYS_READ] = &sys_none,
    [SYS_MKDIR] = &sys_none,
    [SYS_MOUNT] = &sys_none,
    [SYS_CHDIR] = &sys_none,
    [SYS_LSEEK64] = &sys_none,
    [SYS_IOCTL] = &sys_none,
    [SYS_SYNC] = &sys_none
};

void syscall_handler(TRAP_FRAME* trap_frame) {

    // enable interrupt because the lab require in lab5 kernel preemption
    enable_interrupt();

    U64 syscall_index = trap_frame->regs[8];

    //NS_DPRINT("[SYSCALL][TRACE] index: %d\n", syscall_index);
    if (syscall_index >= NR_SYSCALLS) {
        printf("[SYSCALL][ERROR] Invalid system call.\n");
        return;
    }

    (syscall_table[syscall_index])(trap_frame);
}

void sys_none(TRAP_FRAME* tf) {
    // do nothing
}