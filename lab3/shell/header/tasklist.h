#ifndef TASKLIST_H
#define TASKLIST_H

#include <stddef.h>
#include <stdint.h>

typedef void (*task_callback)();


// double linked list store每個tasks並確定每個任務的優先順序, 然後優先執行任務。
typedef struct task {
    struct task *prev;
    struct task *next;
    task_callback callback;
    uint64_t priority;
} task_t;

void execute_tasks();
void create_task(task_callback callback,uint64_t priority);
void enqueue_task(task_t *new_task);
extern task_t *task_head;

#endif
