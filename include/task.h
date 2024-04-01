#ifndef _TASK_H
#define _TASK_H

#define UART_INT_PRIORITY 5
#define TIMER_INT_PRIORITY 8

typedef void (*task_callback)(void);

typedef struct task {
  struct task* next;
  unsigned int priority;
  task_callback func;
} task;

void add_task(task_callback cb, unsigned prio);
void pop_task();

#endif