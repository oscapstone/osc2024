#ifndef _TASKLIST_H
#define _TASKLIST_H

typedef void (*task_callback_t)(void);

typedef struct task_t {
    struct task_t *prev;
    struct task_t *next;
    task_callback_t callback;
    unsigned long long priority;
} task_t;

void execute_tasks();
void create_task(task_callback_t callback, unsigned long long priority);
void enqueue_task(task_t *task);
void execute_tasks_preemptive();
#endif