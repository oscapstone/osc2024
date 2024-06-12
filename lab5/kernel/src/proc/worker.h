#pragma once

#include "task.h"

typedef struct _WORKER_TASK
{
    void (*callback)(void);
    int priority;
    struct _WORKER_TASK* next;
}WORKER_TASK;

typedef struct _WORKER_MANAGER
{
    WORKER_TASK* task_ptr;
    WORKER_TASK* end_ptr;
    TASK* task_handler;             // the task(process) the worker use
}WORKER_MANAGER;

/**
 * initialize the work process
*/
void worker_init();

/**
 * Add a work to the worker
*/
void worker_add(void (*callback)(void), int priority);

