

#include "base.h"

#include "syscall.h"
#include "utils/printf.h"

SYS_CALL syscall_table[NR_SYSCALLS] = {
    [SYS_GETPID] = &sys_getpid,
    [SYS_UART_READ] = &sys_uartread,
    [SYS_UART_WRITE] = &sys_uartwrite,
    [SYS_EXEC] = &sys_exec,
    [SYS_FORK] = &sys_fork,
    [SYS_EXIT] = &sys_exit,
    [SYS_MBOX_CALL] = &sys_mbox_call,
    [SYS_KILL] = &sys_kill
};

void syscall_handler(TRAP_FRAME* trap_frame) {
    U64 syscall_index = trap_frame->regs[8];

    NS_DPRINT("[SYSCALL][TRACE] index: %d\n", syscall_index);
    if (syscall_index >= NR_SYSCALLS) {
        printf("[SYSCALL][ERROR] Invalid system call.\n");
        return;
    }

    (syscall_table[syscall_index])(trap_frame);
}