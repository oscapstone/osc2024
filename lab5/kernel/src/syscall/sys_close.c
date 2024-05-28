
#include "syscall.h"
#include "proc/task.h"
#include "peripherals/irq.h"
#include "fs/fs.h"

void sys_open(TRAP_FRAME* regs) {
    
    TASK* current_task = task_get_current_el1();

    int fd = (int)regs->regs[0];

    if (fd > MAX_FILE_DESCRIPTOR || fd < 0) {
        NS_DPRINT("[SYSCALL][CLOSE] illegal file descriptor id. fd: %d\n", fd);
        regs->regs[0] = -1;
        return;
    }
    FILE_DESCRIPTOR* descriptor = &current_task->file_table[fd];

    if (!descriptor->file) {
        NS_DPRINT("[SYSCALL][CLOSE] file descriptor not open yet. fd: %d\n", fd);
        regs->regs[0] = -1;
        return;
    }

    lock_interrupt();
    vfs_close(descriptor->file);
    descriptor->file = NULL;
    unlock_interrupt();

    // success
    regs->regs[0] = 0;
}
