#include <stddef.h>
#include <stdint.h>

typedef void (*task_callback)();

typedef struct task {
    struct task *prev;
    struct task *next;
    task_callback callback;
    uint64_t task_num;
    uint64_t priority;
    uint64_t elr;
    uint64_t spsr;
} task_t;

void execute_tasks(uint64_t elr,uint64_t spsr);
void create_task(task_callback callback,uint64_t priority);
void enqueue_task(task_t *new_task);
extern task_t *task_head;