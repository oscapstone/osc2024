
#include "task.h"
#include "mm/mm.h"
#include "utils/printf.h"
#include "utils/utils.h"
#include "peripherals/irq.h"

TASK_MANAGER* task_manager;

void task_init() {
    NS_DPRINT("task_init start.\n");


    task_manager = kmalloc(sizeof(TASK_MANAGER));

    for (U32 i = 0; i < TASK_MAX_TASKS; i++) {
        TASK* task = &task_manager->tasks[i];

        task->flags = 0;
        task->pid = i;
    }

    // initialize the kernel init process(task)
    TASK* init = &task_manager->tasks[0];
    init->flags = TASK_FLAGS_RUNNING | TASK_FLAGS_ALLOC | TASK_FLAGS_KERNEL;
    init->kernel_stack = NULL;
    
    // set the tpidr_el1 register to init
    utils_write_sysreg(tpidr_el1, init);

    task_manager->count = 1;            // current alloc tasks
    task_manager->running = 1;          // only init process
    task_manager->running_queue[0] = init;
    NS_DPRINT("task_init end.\n");
    
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

    //NS_DPRINT("[TASK][TRACE] next task pid: %d\n", task_manager->running_queue[0]->pid);
	irq_restore(flags);
    if (current_task == task_manager->running_queue[0]) {
        return;
    }
    task_switch_to(current_task, task_manager->running_queue[0]);
}

void task_run(TASK* task) {
    // 先這樣
    task->flags |= TASK_FLAGS_RUNNING;
    task->preempt = task->priority;

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
}

void task_stop(TASK* task) {
    task->flags &= ~TASK_FLAGS_RUNNING;
    pid_t i = task->pid;
    for (;i < task_manager->running; i++) {
        if (task_manager->running_queue[i + 1] == NULL) {
            task_manager->running_queue[i] = NULL;
            break;
        }
        task_manager->running_queue[i] = task_manager->running_queue[i + 1];
    }
    task_manager->running--;
}

void task_delete(TASK* task) {
    kfree(task->kernel_stack);
    task->flags &= ~TASK_FLAGS_ALLOC;
    task_manager->count--;
}

TASK* task_create(void* func) {

    TASK* task = NULL;
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
    task->flags = TASK_FLAGS_ALLOC; // only allocate, not running
    task->kernel_stack = kmalloc(TASK_STACK_SIZE); // allocate the stack
    task->cpu_regs.lr = (U64)func;
    task->cpu_regs.sp = (U64)((char*)task->kernel_stack + TASK_STACK_SIZE);
    task->cpu_regs.fp = task->cpu_regs.sp;
    task->priority = 0;              // default value
    task->preempt = task->priority;              // default value

    task_manager->count++;
    NS_DPRINT("[TASK][TRACE] task allocated. pid = %d\n", task->pid);

    return task;
}
