#include "task.h"
#include "alloc.h"
#include "irq.h"
#include "entry.h"
#include "../peripherals/mini_uart.h"

task_queue* t_priority_queue[MIN_PRIORITY + 1];
int current_priority;

void init_task_queue(void) {
    for (int i = 0; i <= MIN_PRIORITY; i++) {
        t_priority_queue[i] = (task_queue *)simple_malloc(sizeof(task_queue));
        t_priority_queue[i]->front = NULL;
        t_priority_queue[i]->end = NULL;
    }
    current_priority = 10;
}

// Insert the task into queue.
void enqueue_task(task* t) {
    // The priority queue is empty initially.
    if (t_priority_queue[t->priority]->front == NULL) {
        t_priority_queue[t->priority]->front = t;
        t_priority_queue[t->priority]->end = t;
    } else {
        t_priority_queue[t->priority]->end->next = t;
        t_priority_queue[t->priority]->end = t_priority_queue[t->priority]->end->next;
    }
    t->next = NULL;
}

// Dequeue the task with highest priority.
task* dequeue_task(void) {
    for (int i = 0; i <= MIN_PRIORITY; i++) {
        if (t_priority_queue[i]->front != NULL) {
            task* tmp = t_priority_queue[i]->front;
            t_priority_queue[i]->front = t_priority_queue[i]->front->next;
            if (t_priority_queue[i]->front == NULL) {
                t_priority_queue[i]->end = NULL;
            }

            return tmp;
        }
    }

    return NULL;
}

void execute_task() {
    disable_el1_interrupt();
    // Iterate through the task queues and execute the tasks from high priority to
    // low priority.
    task* t = NULL;
    for (int i = 0; i <= MIN_PRIORITY; i++) {
        if (t_priority_queue[i]->front != NULL) {
            if (t_priority_queue[i]->front->priority < current_priority) {
                t = dequeue_task();
                break;
            }
        }
    }

    if (t != NULL) {
        // If the task has higher priority, execute it first.
        int prev = current_priority;
        current_priority = t->priority;
        enable_el1_interrupt();
        t->intr_func(t->arg);
        current_priority = prev;
        // Check the queue recursively until no task is left.
        execute_task();
    }
    enable_el1_interrupt();
}

// Store the context of current task.
void save_context(task* t) {
    asm volatile (
        //  stp(Store Pair of registers)
        "stp x0, x1, [%0, 16 * 0];"
        "stp x2, x3, [%0, 16 * 1];"
        "stp x4, x5, [%0, 16 * 2];"
        "stp x6, x7, [%0, 16 * 3];"
        "stp x8, x9, [%0, 16 * 4];"
        "stp x10, x11, [%0, 16 * 5];"
        "stp x12, x13, [%0, 16 * 6];"
        "stp x14, x15, [%0, 16 * 7];"
        "stp x16, x17, [%0, 16 * 8];"
        "stp x18, x19, [%0, 16 * 9];"
        "stp x20, x21, [%0, 16 * 10];"
        "stp x22, x23, [%0, 16 * 11];"
        "stp x24, x25, [%0, 16 * 12];"
        "stp x26, x27, [%0, 16 * 13];"
        "stp x28, x29, [%0, 16 * 14];"
        "mrs x0, spsr_el1;"
        "stp x30, x0, [%0, 16 * 15];"
        "mrs x0, elr_el1;"
        "str x0, [%0, 16 * 16];"
        "ldp x0, x1, [%0, 16 * 0];"

        // Output
        :
        // Input
        : "r" (t->sp)
        :
    );
}

void load_context(task* t) {
    asm volatile (
        "ldp x0, x1, [%0, 16 * 0];"
        "ldp x2, x3, [%0, 16 * 1];"
        "ldp x4, x5, [%0, 16 * 2];"
        "ldp x6, x7, [%0, 16 * 3];"
        "ldp x8, x9, [%0, 16 * 4];"
        "ldp x10, x11, [%0, 16 * 5];"
        "ldp x12, x13, [%0, 16 * 6];"
        "ldp x14, x15, [%0, 16 * 7];"
        "ldp x16, x17, [%0, 16 * 8];"
        "ldp x18, x19, [%0, 16 * 9];"
        "ldp x20, x21, [%0, 16 * 10];"
        "ldp x22, x23, [%0, 16 * 11];"
        "ldp x24, x25, [%0, 16 * 12];"
        "ldp x26, x27, [%0, 16 * 13];"
        "ldp x28, x29, [%0, 16 * 14];"
        "ldp x30, x0, [%0, 16 * 15];"
        "msr spsr_el1, x0;"
        "ldr x0, [%0, 16 * 16];"
        "msr elr_el1, x0;"
        "ldp x0, x1, [%0, 16 * 0];"

        :
        : "r" (t->sp)
        :
    );
}