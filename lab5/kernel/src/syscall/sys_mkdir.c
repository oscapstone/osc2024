
#include "syscall.h"
#include "proc/task.h"
#include "peripherals/irq.h"
#include "fs/fs.h"
#include "utils/printf.h"

void sys_mkdir(TRAP_FRAME* regs) {
    
    TASK* current_task = task_get_current_el1();

    const char* pathname = (const char*) regs->regs[0];
    //unsigned mode = (unsigned)regs->regs[1];

    FS_VNODE* parent;
    FS_VNODE* target;
    char tmp_name[FS_MAX_NAME_SIZE];
    int result = fs_find_node(current_task->pwd, pathname, &parent, &target, tmp_name);

    if (result == 0) {
        NS_DPRINT("[SYSCALL][MKDIR] file already exist.\n");
        regs->regs[0] = -1;
        return;
    }

    if (result != FS_FIND_NODE_HAS_PARENT_NO_TARGET) {
        NS_DPRINT("[SYSCALL][MKDIR] parent directory not found.\n");
        regs->regs[0] = -1;
        return;
    }

    if (parent->v_ops->mkdir(parent, &target, tmp_name)) {
        NS_DPRINT("[SYSCALL][MKDIR] Failed to create directory.\n");
        regs->regs[0] = -1;
        return;
    }

    regs->regs[0] = 0;
    return;
}
