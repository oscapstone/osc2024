
#include "syscall.h"
#include "proc/task.h"
#include "peripherals/irq.h"
#include "fs/fs.h"

void sys_open(TRAP_FRAME* regs) {
    
    TASK* current_task = task_get_current_el1();

    const char* pathname = (const char*)regs->regs[0];
    int flags = (int)regs->regs[1];

    lock_interrupt();
    int fd = -1;
    for (int i = 0; i < MAX_FILE_DESCRIPTOR; i++) {
        FILE_DESCRIPTOR* descriptor = &current_task->file_table[i];
        if (descriptor->file == NULL) {
            fd = i;
            break;
        }
    }
    if (fd == -1) {
        NS_DPRINT("[SYSCALL][OPEN] Failed to register file descriptor.\n");
        regs->regs[0] = -1;
        return;
    }
    int result = vfs_open(pathname, flags, &current_task->file_table[fd].file);
    if (result) {
        NS_DPRINT("[SYSCALL][OPEN] Failed to open file: %s\n", pathname);
        current_task->file_table[fd].file = NULL;
        regs->regs[0] = -1;
        return;
    }
    unlock_interrupt();

    regs->regs[0] = fd;
}
