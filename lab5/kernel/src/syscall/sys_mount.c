
#include "syscall.h"
#include "proc/task.h"
#include "peripherals/irq.h"
#include "fs/fs.h"
#include "utils/printf.h"
#include "mm/mm.h"

void sys_mount(TRAP_FRAME* regs) {
    NS_DPRINT("[SYSCALL][MOUNT] start.\n");
    
    TASK* current_task = task_get_current_el1();

    const char* src = (const char*)regs->regs[0];
    const char* target = (const char*) regs->regs[1];
    const char* filesystem = (const char*)regs->regs[2];
    unsigned long flags = (unsigned long) regs->regs[3];
    const void* data = (const void*)regs->regs[4];

    NS_DPRINT("file system addr: %08x%08x\n", (U64)filesystem >> 32, filesystem);

    FS_FILE_SYSTEM* fs = fs_get(filesystem);

    if (!fs) {
        NS_DPRINT("[SYSCALL][MKDIR] file system type not found.\n");
        regs->regs[0] = -1;
        return;
    }

    FS_VNODE* parent_node;
    FS_VNODE* target_node;
    char tmp_name[FS_MAX_NAME_SIZE];
    int result = fs_find_node(current_task->pwd, target, &parent_node, &target_node, tmp_name);

    if (result) {
        NS_DPRINT("[SYSCALL][MKDIR] target to mount not found.\n");
        regs->regs[0] = -1;
        return;
    }

    FS_MOUNT* mount = kzalloc(sizeof(FS_MOUNT));
    mount->fs = fs;
    mount->root = target_node;

    if (fs->setup_mount(fs, mount)) {
        NS_DPRINT("[SYSCALL][MOUNT] failed to mount filesystem. path: %s, filesystem: %s\n", target, filesystem);
        regs->regs[0] = -1;
        return;
    }


    regs->regs[0] = 0;
    NS_DPRINT("[SYSCALL][MOUNT] success.\n");
    return;
}
