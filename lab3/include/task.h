#ifndef _TASK_H_
#define _TASK_H_

#include "exception.h"
#include "list.h"
#include "timer.h"
#include "uart.h"

typedef void (*task_callback_t)(void);

typedef struct {
    int priority;
    task_callback_t callback;
    struct list_head list;
} task_node;

void add_task(int priority, task_callback_t callback);
void pop_task();

#endif