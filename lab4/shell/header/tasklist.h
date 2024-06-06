#ifndef TASKLIST_H
#define TASKLIST_H

// #include <stddef.h>
#include <stdint.h>

#ifdef _cplusplus
#define NULL 0
#else
#define NULL (void*)0
#endif

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ long unsigned int
#endif
// #if !(defined (__GNUG__) && defined (size_t))
typedef __SIZE_TYPE__ size_t;

typedef void (*task_callback)();

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
