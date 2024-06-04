
#include "syscall.h"
#include "arm/mmu.h"
#include "proc/task.h"
#include "fs/fs.h"
#include "utils/printf.h"
#include "peripherals/irq.h"
#include "mm/mm.h"
#include "utils/utils.h"

void sys_exec(TRAP_FRAME* regs) {
    char* name = (char*) regs->regs[0];
    //const char** argv = (const char**) regs->regs[1];
    NS_DPRINT("[SYSCALL][EXEC] Start executing %s\n", name);

    size_t name_len = utils_strlen(name);
    char tmp_name[256];
    memcpy(name, tmp_name, name_len);

    TASK* current_task = task_get_current_el1();

    FS_VNODE* target_vnode;
    if (vfs_lookup(current_task->pwd, tmp_name, &target_vnode)) {
        NS_DPRINT("[SYSCALL][EXEC] Failed to open file: %s\n", tmp_name);
        //NS_DPRINT("[SYSCALL][EXEC] parent directory: %s\n", current_task->pwd->name);
        regs->regs[0] = -1;
        return;
    }

    U64 irq_flags = irq_disable();

    asm volatile("dsb ish\n\t");
    // clean all user program page
    mmu_delete_mm(current_task);
    asm("tlbi vmalle1is\n\t" // invalidate all TLB entries
        "dsb ish\n\t"        // ensure completion of TLB invalidatation
        "isb\n\t");          // clear pipeline
    // init the stack
    mmu_task_init(current_task);

    signal_task_init(current_task);

    // TODO: load a little form file, and do translation page fault during program execution (demand paging)
    // this is not 
    task_run_program(current_task->pwd, current_task, tmp_name);

    regs->pc = MMU_USER_ENTRY;
    regs->sp = MMU_USER_STACK_BASE;
    regs->regs[0] = 0;
    
    irq_restore(irq_flags);

}
