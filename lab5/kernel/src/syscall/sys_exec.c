
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
    // clean all user program page
    mmu_delete_mm(current_task);
    // init the stack
    mmu_task_init(current_task);

    // TODO: load a little form file, and do translation page fault during program execution (demand paging)
    // this is not 
    task_run_program(current_task->pwd, current_task, tmp_name);

    NS_DPRINT("[SYSCALL][EXEC] try jumping to program.\n");
    task_get_current_el1()->cpu_regs.sp = MMU_USER_STACK_BASE;
    task_get_current_el1()->cpu_regs.fp = MMU_USER_STACK_BASE;

    U64 kernel_stack_ptr = (U64)task_get_current_el1()->kernel_stack + TASK_STACK_SIZE - sizeof(TRAP_FRAME);
    
    asm volatile ("msr tpidr_el1, %0\n\t"
        "msr elr_el1, %1\n\t"       // user start code
        "msr spsr_el1, xzr\n\t"     // enable interrupt
        "msr sp_el0, %2\n\t"
        "mov sp, %3\n\t"
        "dsb ish\n\t"
        "msr ttbr0_el1, %4\n\t"
        "tlbi vmalle1is\n\t"
        "dsb ish\n\t"
        "isb\n\t"
        :
        :   "r"(task_get_current_el1()),
            "r"(MMU_USER_ENTRY),
            "r"(task_get_current_el1()->cpu_regs.sp),
            "r"(kernel_stack_ptr),
            "r"(task_get_current_el1()->cpu_regs.pgd)
        );
    
    irq_restore(irq_flags);

    asm volatile(
        "eret\n\t"
        );
}
