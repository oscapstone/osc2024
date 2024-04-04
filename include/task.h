#ifndef _TASK_H
#define _TASK_H

#include "types.h"

#define UART_INT_PRIORITY 5
#define TIMER_INT_PRIORITY 8
#define SW_INT_PRIORITY 90
#define NO_TASK 101

typedef void (*task_callback)(void);

typedef struct task {
  struct task* next;
  uint32_t priority;
  task_callback func;
} task;

void add_task(task_callback cb, uint32_t prio);
void pop_task();

#endif