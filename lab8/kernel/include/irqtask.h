#ifndef _IRQTASK_H_
#define _IRQTASK_H_

#include "u_list.h"

// smaller is more preemptive
#define UART_IRQ_PRIORITY  1
#define TIMER_IRQ_PRIORITY 0

typedef struct irqtask
{
    struct list_head listhead;
    unsigned long long priority; // store priority (smaller number is more preemptive)
    void *task_function;         // task function pointer
} irqtask_t;

void irqtask_add(void *task_function, unsigned long long priority);
void irqtask_run(irqtask_t *the_task);
void irqtask_run_preemptive();
void irqtask_list_init();

#endif
