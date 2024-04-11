#ifndef TASKLIST_H
#define TASKLIST_H

typedef void(*task_callback)(void);

typedef struct task {
    struct task *next;
    task_callback callback;
    int p;
} task;

void execute_tasks();
void create_task(task_callback callback,int priority);
void enqueue_task(task *new_task);

#endif
