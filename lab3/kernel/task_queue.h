#ifndef __IRQ_TASK_QUEUE_H__
#define __IRQ_TASK_QUEUE_H__
void init_task_queue();

void add_task(void (*func)(void), int priority);

void run_task();

#endif
