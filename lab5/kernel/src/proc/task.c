
#include "task.h"
#include "mm/mm.h"
#include "utils/printf.h"
#include "utils/utils.h"
#include "peripherals/irq.h"
#include "arm/mmu.h"
#include "peripherals/timer.h"

TASK_MANAGER* task_manager;

void task_to_user_func();
void task_timer_callback();

void task_init() {
    NS_DPRINT("task_init start.\n");

    task_manager = kzalloc(sizeof(TASK_MANAGER));

    for (U32 i = 0; i < TASK_MAX_TASKS; i++) {
        TASK* task = &task_manager->tasks[i];

        task->flags = 0;
        task->pid = i;
        task->parent_pid = 0;       // set default parent pid to init
    }

    // initialize the kernel init process(task)
    TASK* init = &task_manager->tasks[0];
    init->status = TASK_STATUS_RUNNING;
    init->flags = TASK_FLAGS_ALLOC | TASK_FLAGS_KERNEL;
    init->kernel_stack = NULL;
    init->cpu_regs.pgd = PD_KERNEL_ENTRY;
    utils_char_fill(init->name, "init", 4);
    signal_task_init(init);
    
    // set the tpidr_el1 register to init
    utils_write_sysreg(tpidr_el1, init);

    task_manager->count = 1;            // current alloc tasks
    task_manager->running = 1;          // only init process
    task_manager->running_queue[0] = init;
    task_manager->dead_count = 0;

    // add timer to schedule
    timer_add(task_timer_callback, TASK_SCHE_FREQ);
    NS_DPRINT("task_init end.\n");
    
}

void task_timer_callback() {

    timer_add(task_timer_callback, TASK_SCHE_FREQ);
    enable_interrupt();
    task_schedule();
}

void task_schedule() {
    
    U64 flags = irq_disable();
    if (task_manager->running == 0) {
        printf("[TASK][FATAL] No running task!\n");
        for (;;) {} // hlt here
    }

    // current task need to move
    TASK* current_task = task_manager->running_queue[0];
    //NS_DPRINT("[TASK][TRACE] current task pid = %d\n", current_task->pid);
    BOOL preempt = FALSE;
    current_task->preempt = current_task->priority;                         // reset the preempt value
    for (U32 i = 1; i < task_manager->running; i++) {
        TASK* iter_task = task_manager->running_queue[i];
        if (iter_task->preempt > current_task->preempt && iter_task->preempt > 0) {
            iter_task->preempt--;
            task_manager->running_queue[i - 1] = current_task;
            preempt = TRUE;
            break;
        }
        //NS_DPRINT("[TASK][TRACE] put task idx %d to %d, pid = %d\n", i, i - 1, iter_task->pid);
        task_manager->running_queue[i - 1] = iter_task;
    }
    if (preempt == FALSE) {
        //NS_DPRINT("[TASK][TRACE] Not preempt put the task to the back\n");
        task_manager->running_queue[task_manager->running - 1] = current_task;
    }
    irq_restore(flags);

    //NS_DPRINT("[TASK][TRACE] next task pid: %d\n", task_manager->running_queue[0]->pid);
    
    if (current_task != task_manager->running_queue[0]) {
        task_switch_to(task_get_current_el1(), task_manager->running_queue[0]);
    }
}

TASK* task_get(int pid) {
    TASK* task = &task_manager->tasks[pid];
    if (!(task->flags & TASK_FLAGS_ALLOC)) {
        return NULL;
    }
    return task;
}

void task_switch_to(TASK* current, TASK* next) {
    enable_interrupt();
    task_asm_switch_to(current, next);
}


void task_run_to_el0(TASK* task) {
    // 先這樣
    lock_interrupt();
    task->status = TASK_STATUS_RUNNING;
    task->preempt = task->priority;
    task->cpu_regs.lr = task_to_user_func;

    for (U32 i = 0; i < task_manager->running; i++) {
        TASK* run_task = task_manager->running_queue[i];
        if (run_task->preempt > task->preempt) {
            TASK* tmp_task = run_task;
            task_manager->running_queue[i] = task;
            for (U32 j = i; j < task_manager->running; j++) {
                TASK* next_task = task_manager->running_queue[j + 1];
                task_manager->running_queue[j + 1] = tmp_task;
                tmp_task = next_task;
            }
            //NS_DPRINT("[TASK][TRACE] adding task to running queue[%d], pid = %d\n", i, task->pid);
            task_manager->running++;
            return;
        }
    }
    task_manager->running_queue[task_manager->running] = task;
    //NS_DPRINT("[TASK][TRACE] adding task to running queue[%d], pid = %d\n", task_manager->running, task->pid);
    task_manager->running++;
    unlock_interrupt();
    NS_DPRINT("[TASK][TRACE] new task running. pid = %d\n", task->pid);
}

void task_add_to_run_list(TASK* task) {
    U64 flags = irq_disable();
    task->status = TASK_STATUS_RUNNING;
    task->preempt = task->priority;
    task->current_signal = -1;      // no signal starting run

    for (U32 i = 0; i < task_manager->running; i++) {
        TASK* run_task = task_manager->running_queue[i];
        if (run_task->preempt > task->preempt) {
            TASK* tmp_task = run_task;
            task_manager->running_queue[i] = task;
            for (U32 j = i; j < task_manager->running; j++) {
                TASK* next_task = task_manager->running_queue[j + 1];
                task_manager->running_queue[j + 1] = tmp_task;
                tmp_task = next_task;
            }
            //NS_DPRINT("[TASK][TRACE] adding task to running queue[%d], pid = %d\n", i, task->pid);
            task_manager->running++;
            return;
        }
    }
    task_manager->running_queue[task_manager->running++] = task;
    //NS_DPRINT("[TASK][TRACE] adding task to running queue[%d], pid = %d\n", task_manager->running, task->pid);
    irq_restore(flags);
}

void task_run(TASK* task) {
    if (!(task->flags & TASK_FLAGS_ALLOC)) {
        NS_DPRINT("[TASK][ERROR] task not allcated.");
        return;
    }
    task_add_to_run_list(task);
}

void task_to_user_func() {

    task_get_current_el1()->cpu_regs.sp = MMU_USER_STACK_BASE;
    task_get_current_el1()->cpu_regs.fp = MMU_USER_STACK_BASE;

    UPTR program_entry = 0x0L;

    asm volatile("msr tpidr_el1, %0\n\t"
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
            "r"(program_entry),
            "r"(task_get_current_el1()->cpu_regs.sp),
            "r"(task_get_current_el1()->kernel_stack + TASK_STACK_SIZE),
            "r"(task_get_current_el1()->cpu_regs.pgd)
        );
}

void task_exit(int exitcode) {
    task_kill(task_get_current_el1()->pid, exitcode);
}

void task_remove_from_run_list(TASK* task) {
    U32 i = 0;
    for(;i < task_manager->running;i++) {
        if (task_manager->running_queue[i] == task)
            break;
    }
    for (;i < task_manager->running; i++) {
        if (task_manager->running_queue[i + 1] == NULL) {
            task_manager->running_queue[i] = NULL;
            break;
        }
        task_manager->running_queue[i] = task_manager->running_queue[i + 1];
    }
    task_manager->running--;
}

int task_kill(pid_t pid, int exitcode) {
    if (pid == 0) {
        printf("You can not kill the init process.\n");
        return -1;
    }

    TASK* task = &task_manager->tasks[pid];
    if (task->status != TASK_STATUS_RUNNING) {
        printf("[TASK][WARN] task does not run. pid = %d, status = %d\n", pid, task->status);
        return -1;
    }
    lock_interrupt();
    task_remove_from_run_list(task);
    task->exitcode = exitcode;
    
    task->status = TASK_STATUS_DEAD;

    task_manager->dead_list[task_manager->dead_count++] = task;
    
    // TODO: Signal the parent task to know by waiting
    // exitcode

    unlock_interrupt();
    NS_DPRINT("[TASK][TRACE] task killed. pid = %d\n", task->pid);
    if (task_get_current_el1() == task)
        task_schedule();
    return 0;
}

void task_delete(TASK* task) {
    lock_interrupt();
    mmu_delete_mm(task);            // free page table
    if (task->kernel_stack) {
        kfree(task->kernel_stack);
        task->kernel_stack = NULL;
    }
    task->status = 0;
    task->flags = 0;
    task_manager->count--;
    unlock_interrupt();

    // clean up the file that had been allocated.
    for (U32 i = 0; i < MAX_FILE_DESCRIPTOR; i++) {
        FILE_DESCRIPTOR* descriptor = &task->file_table[i];
        if (descriptor->file) {
            vfs_close(descriptor->file);
            descriptor->file = NULL;
        }
    }
    NS_DPRINT("[TASK][TRACE] task deleted. pid = %d\n", task->pid);
}

void task_kill_dead() {
    disable_interrupt();
    for (U32 i = 0; i < task_manager->dead_count; i++) {
        task_delete(task_manager->dead_list[i]);
        task_manager->dead_list[i] = NULL;
    }
    task_manager->dead_count = 0;
    enable_interrupt();
}

// common task for task creation not called by any system
TASK* task_alloc(const char* name, U32 flags) {
    TASK* task = NULL;
    U64 irq_flags = irq_disable();
    for (U32 i = 0; i < TASK_MAX_TASKS; i++) {
        if (!(task_manager->tasks[i].flags & TASK_FLAGS_ALLOC)) {
            task = &task_manager->tasks[i];
            break;
        }
    }
    if (task == NULL) {
        printf("[TASK][ERROR] Failed to create task.\n");
        return NULL;
    }
    task->flags = TASK_FLAGS_ALLOC | flags; // only allocate, not running
    irq_restore(irq_flags);

    task->kernel_stack = kzalloc(TASK_STACK_SIZE); // allocate the stack

    // map the stack to page table
    mmu_task_init(task);

    task->cpu_regs.lr = 0;
    task->cpu_regs.sp = (U64)((char*)task->kernel_stack + TASK_STACK_SIZE);
    task->cpu_regs.fp = task->cpu_regs.sp;
    task->priority = 0;              // default value
    task->preempt = task->priority;              // default value
    utils_char_fill(task->name, name, utils_strlen(name));

    // init signal
    signal_task_init(task);

    task->pwd = fs_get_root_node();     // just use the root node now for user process, will change if this user process load by tty

    task_manager->count++;
    NS_DPRINT("[TASK][TRACE] task allocated. pid = %d\n", task->pid);

    return task;
}

TASK* task_create_user(const char* name, U32 flags) {
    TASK* task = task_alloc(name, flags);
    if (!task) {
        return NULL;
    }

    return task;
}

TASK* task_create_kernel(const char* name, U32 flags) {
    TASK* task = task_alloc(name, flags | TASK_FLAGS_KERNEL);
    if (!task) {
        return NULL;
    }
    task->cpu_regs.pgd = PD_KERNEL_ENTRY;           // kernel process using just kernel paging

    return task;
}

void task_copy_program(TASK* task, void* program_start, size_t program_size) {
    size_t offset = 0;
    NS_DPRINT("[TASK][DEBUG] copy program. program size: %d bytes\n", program_size);
    while (offset < program_size) {
        size_t size = offset + PD_PAGE_SIZE > program_size ? program_size - offset : PD_PAGE_SIZE;

        U64 page = kzalloc(PD_PAGE_SIZE);
        memcpy((char*)program_start + offset, (void*)page, size);
        mmu_map_page(task, offset, MMU_VIRT_TO_PHYS(page), MMU_AP_EL0_UK_ACCESS | MMU_PXN);

        offset += size;
    }
    NS_DPRINT("[TASK][DEBUG] Task copied. pid = %d\n", task->pid);
}

void task_wait(pid_t pid) {
    while (1) {
        if (task_manager->tasks[pid].status != TASK_STATUS_RUNNING) {
            break;
        }
        task_schedule();
    }
}

/**
 * Remove from running queue, called by current task(process)
*/
void task_sleep() {
    disable_interrupt();
    task_remove_from_run_list(task_get_current_el1());
    task_get_current_el1()->status = TASK_STATUS_SLEEPING;
    task_schedule();
}

void task_awake(pid_t pid) {
    if (task_manager->tasks[pid].status != TASK_STATUS_SLEEPING)
        return;
    task_add_to_run_list(&task_manager->tasks[pid]);
}

BOOL task_is_running(TASK* task) {
    return task->status == TASK_STATUS_RUNNING;
}
pid_t task_get_pid(TASK* task) {
    return task->pid;
}