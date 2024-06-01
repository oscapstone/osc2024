

#include "task.h"
#include "signal.h"
#include "mm/mm.h"
#include "peripherals/irq.h"
#include "syscall/syscall.h"
#include "utils/utils.h"
#include "utils/printf.h"
#include "arm/mmu.h"

void signal_run(TRAP_FRAME* trap_frame, int signal);
void signal_entry();

/**
 * in every irq handler check if it has signal in current process
*/
void signal_check(TRAP_FRAME* trap_frame) {
    TASK* task = task_get_current_el1();
    lock_interrupt();
    if (task->current_signal != -1) {   // is running a signal now
        unlock_interrupt();
        return;
    }
    unlock_interrupt();

    for (int i = 0; i < SIGNAL_NUM; i++) {
        TASK_SIGNAL* signal = &task->signals[i];
        if (signal->count > 0) {
            lock_interrupt();
            signal->count--;
            task_get_current_el1()->current_signal = i;
            unlock_interrupt();
            signal_run(trap_frame, i);
        }
    }
}

/**
 * Signal a handler
 * @param task
 *      the task that will be signal
 * @param signal
 *      signal number
*/
void signal_run(TRAP_FRAME* trap_frame, int signal) {
    TASK_SIGNAL* current_signal = &task_get_current_el1()->signals[signal];

    if (signal == SIGNAL_KILL && !current_signal->handler) {
        task_exit(-2);  // kill by other process
    }

    if (!current_signal->handler) {
        lock_interrupt();
        task_get_current_el1()->current_signal = -1;
        unlock_interrupt();
        printf("[SYSCALL][SIGNAL][WARN] no signal handler. pid: %d, signal: %d\n", task_get_current_el1()->pid, signal);
        return;
    }

    // allocate signal
    current_signal->cpu_regs = kzalloc(sizeof(CPU_REGS));
    current_signal->stack = kzalloc(TASK_STACK_SIZE);

    // save the current context
    task_asm_store_context(current_signal->cpu_regs);

    if (task_get_current_el1()->current_signal == -1) {
        // signal exited. return
        return;
    }

    lock_interrupt();
    current_signal->sp0 = utils_read_sysreg(sp_el0);

    // replace the user stack in page table
    U64 stack_ptr = (U64)MMU_VIRT_TO_PHYS((U64)current_signal->stack);
    U64 stack_v_addr = MMU_USER_STACK_BASE - TASK_STACK_SIZE;
    for (int i = 0; i < TASK_STACK_PAGE_COUNT; i++) {
        U64 pte = mmu_get_pte(task_get_current_el1(), stack_v_addr);
        mmu_map_table_entry((pd_t*)(pte + 0), stack_v_addr, stack_ptr + (i * PD_PAGE_SIZE), MMU_AP_EL0_UK_ACCESS | MMU_UNX /* stack為不可執行 */ | MMU_PXN/* 還沒看懂到底要不要家: 要加因為user stack不可在EL1執行*/);
        stack_v_addr += PD_PAGE_SIZE;
    }
    unlock_interrupt();

    NS_DPRINT("[SYSCALL][SIGNAL] doing signal. pid: %d, signal: %d\n", task_get_current_el1()->pid, signal);
    UPTR entry_ptr = MMU_SINGAL_ENTRY_BASE + ((UPTR)signal_entry & 0xfff);
    asm volatile(
        "msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "msr spsr_el1, xzr\n\t"
        "mov x0, %2\n\t"
        "eret\n\t"
        :
        : "r"(entry_ptr),        // 我map兩的page
          "r"(MMU_USER_STACK_BASE),
          "r"(current_signal->handler)
    );

}

void signal_entry() {
    asm volatile(
        "blr x0\n\t"
    );
    asm volatile(
        "mov x8, %0\n\t"
        "svc 0\n\t"
        :
        : "r"(SYS_SIGRETURN)
    );
}

void signal_exit() {
    TASK* task = task_get_current_el1();
    if (task->current_signal == -1) {
        // wired
        printf("[ERROR] signal exit with wired signal number.\n");
        task_exit(-1);
    }
    TASK_SIGNAL* current_signal = &task->signals[task->current_signal];

    // restore the user stack in page table
    lock_interrupt();
    U64 stack_ptr = (U64)MMU_VIRT_TO_PHYS((U64)task->user_stack);
    U64 stack_v_addr = MMU_USER_STACK_BASE - TASK_STACK_SIZE;
    for (int i = 0; i < TASK_STACK_PAGE_COUNT; i++) {
        U64 pte = mmu_get_pte(task, stack_v_addr);
        mmu_map_table_entry((pd_t*)(pte + 0), stack_v_addr, stack_ptr + (i * PD_PAGE_SIZE), MMU_AP_EL0_UK_ACCESS | MMU_UNX /* stack為不可執行 */ | MMU_PXN/* 還沒看懂到底要不要家: 要加因為user stack不可在EL1執行*/);
        stack_v_addr += PD_PAGE_SIZE;
    }
    // free the
    NS_DPRINT("[SIGNAL] goback to process, pid = %d, signal: %d\n", task->pid, task->current_signal);

    task->current_signal = -1;

    // restore current context
    utils_write_sysreg(sp_el0, current_signal->sp0);
    kfree(current_signal->stack);
    current_signal->sp0 = NULL;
    CPU_REGS tmp_reg;
    memcpy(current_signal->cpu_regs, &tmp_reg, sizeof(CPU_REGS));
    kfree(current_signal->cpu_regs);
    unlock_interrupt();
    task_asm_load_context(&tmp_reg);

}