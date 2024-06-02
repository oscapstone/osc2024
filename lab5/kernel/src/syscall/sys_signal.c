
#include "base.h"
#include "syscall.h"
#include "proc/task.h"
#include "peripherals/irq.h"
#include "fs/fs.h"
#include "utils/printf.h"
#include "proc/signal.h"

void sys_signal_register(TRAP_FRAME* regs) {

    int signal = regs->regs[0];
    void (*handler)() = (void(*)())regs->regs[1];

    if (signal >= SIGNAL_NUM || signal < 0) {
        regs->regs[0] = -1;
        return;
    }

    task_get_current_el1()->signals[signal].handler = handler;
    NS_DPRINT("[SYSCALL][SIGNAL] signal registered. pid: %d, signal: %d, handler addr: 0x%x\n", task_get_current_el1()->pid, signal, handler);

    regs->regs[0] = 0;
}

void sys_signal(TRAP_FRAME* trap_frame) {
    int pid = trap_frame->regs[0];
    int signal = trap_frame->regs[1];

    if (pid >= TASK_MAX_TASKS || pid < 0 || signal >= SIGNAL_NUM || signal < 0) {
        trap_frame->regs[0] = -1;
        return;
    }

    TASK* task = task_get(pid);
    if (!task) {
        printf("[SYSCALL][SIGNAL] task not found. pid = %d\n", pid);
        trap_frame->regs[0] = -1;
        return;
    }
    lock_interrupt();
    task->signals[signal].count++;
    NS_DPRINT("[SYSCALL][SIGNAL] signaled. pid: %d, signal: %d, count: %d\n", task->pid, signal, task->signals[signal].count);
    unlock_interrupt();

    trap_frame->regs[0] = 0;
    return;
}

void sys_sigreturn(TRAP_FRAME* trap_frame) {
    signal_exit();
}