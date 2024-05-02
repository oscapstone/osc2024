#ifndef _IRQTASK_H_
#define _IRQTASK_H_

#include "list.h"

// smaller is more preemptive
#define UART_IRQ_PRIORITY  1
#define TIMER_IRQ_PRIORITY 0

typedef struct irqtask
{
    struct list_head listhead;
    unsigned long long priority;
    void *task_function;
} irqtask_t;

void irqtask_add(void *task_function, unsigned long long priority);
void irqtask_run(irqtask_t *the_task);
void irqtask_run_preemptive();
void irqtask_init_list();

#endif
