#ifndef _IRQTASK_H_
#define _IRQTASK_H_

#include "list.h"

#define UART_PRIORITY  1
#define TIMER_PRIORITY 0

typedef struct irqtask
{
    struct list_head listhead;
    unsigned long long priority; // store priority (smaller number is more preemptive)
    void *callback;         // task function pointer
} irqtask_t;

void task_list_init();
void add_task_list(void *callback, unsigned long long priority);
void run_task(irqtask_t *the_task);
void preemption();

#endif