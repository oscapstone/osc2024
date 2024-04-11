#ifndef TASKLIST_H
#define TASKLIST_H

#include <stddef.h>
#include <stdint.h>

typedef void (*task_callback)(void *userdata);

typedef struct task {
    struct task *prev;
    struct task *next;
    task_callback callback;
    void *userdata;
    uint64_t priority;
    char *name;
} task_t;

void execute_tasks();
void create_task(task_callback callback,uint64_t priority,void *data, char *name);
void enqueue_task(task_t *new_task);
extern task_t *task_head;

#endif
