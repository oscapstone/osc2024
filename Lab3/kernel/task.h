#ifndef _TASK_H_
#define _TASK_H_

// There can be maximum of 10 priorities(0 ~ 9).
#define MIN_PRIORITY 9

// Function pointer for the handler function to be executed.
typedef void(*task_func_t)(void *);

typedef struct _task {
    task_func_t intr_func;
    void* data;
    struct _task* next;
} task;

typedef struct _task_queue {
    task* front;
    task* end;
} task_queue;

// Front -> highest priority
// End -> Lowest priority
typedef struct task_priority_queue {
    task_queue* priority_q[MIN_PRIORITY + 1];
} task_p_queue;

extern task_p_queue* t_priority_queue;

void enqueue_task(task* t, int priority);

#endif