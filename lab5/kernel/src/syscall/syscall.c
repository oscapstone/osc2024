

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
    [SYS_SIGNAL_REGISTER] = &sys_signal_register,
    [SYS_SIGNAL] = &sys_signal,
    [SYS_MMAP] = &sys_mmap,
    [SYS_OPEN] = &sys_open,
    [SYS_CLOSE] = &sys_close,
    [SYS_WRITE] = &sys_write,
    [SYS_READ] = &sys_read,
    [SYS_MKDIR] = &sys_mkdir,
    [SYS_MOUNT] = &sys_mount,
    [SYS_CHDIR] = &sys_chdir,
    [SYS_LSEEK64] = &sys_lseek64,
    [SYS_IOCTL] = &sys_ioctl,
    [SYS_SYNC] = &sys_sync,
    [SYS_SIGRETURN] = &sys_sigreturn
};

void syscall_handler(TRAP_FRAME* trap_frame) {

    U64 syscall_index = trap_frame->regs[8];

    //NS_DPRINT("[SYSCALL][TRACE] index: %d\n", syscall_index);
    if (syscall_index >= NR_SYSCALLS) {
        printf("[SYSCALL][ERROR] Invalid system call.\n");
        return;
    }

    if (syscall_index != SYS_WRITE)
        enable_interrupt();

    (syscall_table[syscall_index])(trap_frame);
}

void sys_none(TRAP_FRAME* tf) {
    tf->regs[0] = -1;
}