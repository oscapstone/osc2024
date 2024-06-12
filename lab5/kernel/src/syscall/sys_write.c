
#include "syscall.h"
#include "proc/task.h"
#include "peripherals/irq.h"
#include "fs/fs.h"
#include "utils/printf.h"

void sys_write(TRAP_FRAME* regs) {
    //disable_interrupt();
    //NS_DPRINT("[SYSCALL][WRITE] start.\n");
    U64 irq_flags = irq_disable();
    TASK* task = task_get_current_el1();

    int fd = (int)regs->regs[0];
    void* buf = (void*)regs->regs[1];
    size_t count = regs->regs[2];

    if (fd > MAX_FILE_DESCRIPTOR || fd < 0) {
        NS_DPRINT("[SYSCALL][CLOSE] illegal file descriptor id. fd: %d\n", fd);
        regs->regs[0] = -1;
        return;
    }
    FS_FILE* file = task->file_table[fd].file;

    if (!file) {
        NS_DPRINT("[SYSCALL][CLOSE] file descriptor not open yet. fd: %d\n", fd);
        regs->regs[0] = -1;
        return;
    }

    //NS_DPRINT("[FS] write addr: 0x%p\n", file->vnode);
    regs->regs[0] = vfs_write(file, buf, count);
    //NS_DPRINT("[SYSCALL][WRITE] end. result: %d\n", regs->regs[0]);
    irq_restore(irq_flags);
}