
#include "syscall.h"
#include "proc/task.h"
#include "peripherals/irq.h"
#include "fs/fs.h"
#include "utils/printf.h"

void sys_chdir(TRAP_FRAME* regs) {

    TASK* task = task_get_current_el1();

    const char* path = (const char*) regs->regs[0];
    NS_DPRINT("[SYSCALL][CHDIR] try change to %s.\n", path);

    FS_VNODE* parent;
    FS_VNODE* target;
    char node_name[FS_MAX_NAME_SIZE];
    if (fs_find_node(task->pwd, path, &parent, &target, node_name)) {
        NS_DPRINT("[SYSCALL][CHDIR] target directroy not found.\n");
        regs->regs[0] = -1;
        return;
    }

    if (!S_ISDIR(target->mode)) {
        NS_DPRINT("[SYSCALL][CHDIR] target is not a directory.\n");
        regs->regs[0] = -1;
        return;
    }

    task->pwd = target;
    regs->regs[0] = 0;
}