
#include "syscall.h"
#include "proc/task.h"
#include "peripherals/irq.h"
#include "fs/fs.h"
#include "utils/printf.h"

void sys_read(TRAP_FRAME* regs) {
    //NS_DPRINT("[SYSCALL][READ] start.\n");

    TASK* task = task_get_current_el1();

    int fd = (int)regs->regs[0];
    void* buf = (void*)regs->regs[1];
    size_t count = regs->regs[2];

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

    regs->regs[0] = vfs_read(descriptor->file, buf, count);
    //NS_DPRINT("[SYSCALL][READ] end. result: %d\n", regs->regs[0]);
}