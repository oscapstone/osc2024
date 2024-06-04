
#include "syscall.h"
#include "proc/task.h"
#include "peripherals/irq.h"
#include "fs/fs.h"
#include "utils/printf.h"
#include "mm/mm.h"

void sys_lseek64(TRAP_FRAME* regs) {
    
    TASK* task = task_get_current_el1();

    int fd = (int)regs->regs[0];
    long offset = (long)regs->regs[1];
    int whence = (int)regs->regs[2];
    //NS_DPRINT("[SYSCALL][LSEEK64] start. offset: %d\n", offset);

    if (fd > MAX_FILE_DESCRIPTOR || fd < 0) {
        NS_DPRINT("[SYSCALL][CLOSE] illegal file descriptor id. fd: %d\n", fd);
        regs->regs[0] = -1;
        return;
    }
    FILE_DESCRIPTOR* descriptor = &task->file_table[fd];

    if (!descriptor->file) {
        NS_DPRINT("[SYSCALL][CLOSE] file descriptor not open yet. fd: %d\n", fd);
        regs->regs[0] = -1;
        return;
    }

    U64 irq_flags = irq_disable();
    regs->regs[0] = descriptor->file->vnode->f_ops->lseek64(descriptor->file, offset, whence);
    //NS_DPRINT("[SYSCALL][LSEEK64] result: %d.\n", regs->regs[0]);
    irq_restore(irq_flags);

    return;
}
