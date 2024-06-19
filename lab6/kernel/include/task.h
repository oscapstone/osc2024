
#ifndef TASK_H
#define TASK_H

#include "list.h"

typedef void (*task_callback_t)(void);

typedef struct task {
    list_head_t listhead;
    int priority;
    task_callback_t callback;
} task_t;

extern list_head_t task_list;
void task_list_init() ;
void add_task(task_callback_t callback, int priority);
void pop_task();


#endif