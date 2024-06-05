
#include "syscall.h"
#include "proc/task.h"
#include "peripherals/irq.h"
#include "fs/fs.h"
#include "utils/printf.h"

void sys_close(TRAP_FRAME* regs) {
    NS_DPRINT("[SYSCALL][CLOSE] start.\n");
    
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

    U64 irq_flags = irq_disable();
    vfs_close(descriptor->file);
    descriptor->file = NULL;
    irq_restore(irq_flags);

    // success
    regs->regs[0] = 0;
    NS_DPRINT("[SYSCALL][CLOSE] success. fd: %d\n", fd);
}
