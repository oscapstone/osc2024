#include "worker.h"
#include "mm/mm.h"
#include "peripherals/irq.h"
#include "utils/printf.h"

WORKER_MANAGER worker_manager;

void worker_task_func() {

    while (TRUE) {
        
        WORKER_TASK* work = worker_manager.task_ptr;
        while (work) {
            work->callback();
            WORKER_TASK* toFree = work;
            work = work->next;
            kfree(toFree);
        }
        U64 irq_flag = irq_disable();
        worker_manager.task_ptr = NULL;
        worker_manager.end_ptr = NULL;
        irq_restore(irq_flag);
        //NS_DPRINT("[WORKER] sleep.");
        task_sleep();
    }

}

void worker_init() {
    worker_manager.task_handler = task_create_kernel("kernel_worker", 0);
    worker_manager.task_handler->cpu_regs.lr = (U64)worker_task_func;
	task_run(worker_manager.task_handler);
}

void worker_add(void (*callback)(void)) {
    WORKER_TASK* work = kzalloc(sizeof(WORKER_TASK));
    work->callback = callback;
    work->next = NULL;

    if (!worker_manager.task_ptr) {
        worker_manager.task_ptr = work;
        worker_manager.end_ptr = work;

        //NS_DPRINT("[WORKER] awake.");
        task_awake(task_get_pid(worker_manager.task_handler));
        return;
    }

    worker_manager.end_ptr->next = work;
    worker_manager.end_ptr = work;

    if (!task_is_running(worker_manager.task_handler)) {
        //NS_DPRINT("[WORKER] awake.");
        task_awake(task_get_pid(worker_manager.task_handler));
    }
}