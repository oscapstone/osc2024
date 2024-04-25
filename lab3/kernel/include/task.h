
#ifndef TASK_H
#define TASK_H

#include "irq.h"

typedef void (*task_callback_t)(void);

typedef struct task {
    list_head_t listhead;
    int priority;
    task_callback_t callback;
} task_t;



void add_task(task_callback_t callback, int priority);
void pop_task();


#endif