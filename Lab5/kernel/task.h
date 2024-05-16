#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>

// There can be maximum of 10 priorities(0 ~ 9).
#define MIN_PRIORITY 9

// Function pointer for the handler function to be executed.
typedef void(*task_func_t)(void *);

typedef struct _task {
    task_func_t intr_func;
    // Argument for interrupt function.
    void* arg;
    int priority;
    struct _task* next;
    // Stores the stack pointer of the current task to keep track in case of nested interrupt.
    uint64_t sp;
} task;

typedef struct _task_queue {
    task* front;
    task* end;
} task_queue;

// Front -> highest priority
// End -> Lowest priority
extern task_queue* priority_q[MIN_PRIORITY + 1];

void save_context(task* t);
void load_context(task* t);

void init_task_queue(void);
void enqueue_task(task* t);
task* dequeue_task(void);
void execute_task();

#endif