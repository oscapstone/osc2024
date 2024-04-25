#ifndef _TASK_H_
#define _TASK_H_

#include "irq.h"
#include "list.h"
#include "timer.h"
#include "uart.h"


typedef struct task
{
    // struct list_head listhead;
    struct task *next;
    struct task *prev;
    int priority; // store priority (smaller number is more preemptive)
    void *task_function;         // task function pointer
} task;
void irqtask_list_init();
void add_task(void *task_function, int priority);
void run_task();

void high_prio();
void low_prio();
void test_preempt();

#endif