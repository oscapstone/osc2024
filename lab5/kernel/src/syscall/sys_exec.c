
#include "syscall.h"
#include "arm/mmu.h"
#include "proc/task.h"
#include "fs/fs.h"
#include "utils/printf.h"
#include "peripherals/irq.h"
#include "mm/mm.h"
#include "utils/utils.h"

void jump_user_prog() {

    lock_interrupt();
    task_get_current_el1()->cpu_regs.sp = MMU_USER_STACK_BASE;
    task_get_current_el1()->cpu_regs.fp = MMU_USER_STACK_BASE;
    unlock_interrupt();

    U64 kernel_stack_ptr = (U64)task_get_current_el1()->kernel_stack + TASK_STACK_SIZE - sizeof(TRAP_FRAME);

    asm("msr tpidr_el1, %0\n\t"
        "msr elr_el1, %1\n\t"       // user start code
        "msr spsr_el1, xzr\n\t"     // enable interrupt
        "msr sp_el0, %2\n\t"
        "mov sp, %3\n\t"
        "dsb ish\n\t"
        "msr ttbr0_el1, %4\n\t"
        "tlbi vmalle1is\n\t"
        "dsb ish\n\t"
        "isb\n\t"
        "eret\n\t"
        :
        :   "r"(task_get_current_el1()),
            "r"(0x0),
            "r"(task_get_current_el1()->cpu_regs.sp),
            "r"(kernel_stack_ptr),
            "r"(task_get_current_el1()->cpu_regs.pgd)
        );
}

void sys_exec(TRAP_FRAME* regs) {
    char* name = (char*) regs->regs[0];
    const char** argv = (const char**) regs->regs[1];
    NS_DPRINT("[SYSCALL][EXEC] Start executing %s\n", name);

    FS_FILE* file;
    int result;
    if (result = vfs_open(task_get_current_el1()->pwd, name, O_READ, &file)) {
        NS_DPRINT("[SYSCALL][EXEC] Failed to open file: %s, return code: %d\n", name, result);
        NS_DPRINT("[SYSCALL][EXEC] parent directory: %s\n", task_get_current_el1()->pwd->name);
        regs->regs[0] = -1;
        return;
    }

    TASK* current_task = task_get_current_el1();

    lock_interrupt();
    // clean all user program page
    mmu_delete_mm(current_task);
    // init the stack
    mmu_task_init(current_task);
    unlock_interrupt();

    // TODO: load a little form file, and do translation page fault during program execution (demand paging)
    // this is not 
    unsigned long contentSize = file->vnode->content_size;
    char programName[20];
    utils_char_fill(programName, file->vnode->name, utils_strlen(file->vnode->name));
    char* buf = kmalloc(file->vnode->content_size);
    vfs_read(file, buf, contentSize);
    vfs_close(file);

    task_copy_program(current_task, buf, contentSize);
    kfree(buf);
    jump_user_prog();

    
}
