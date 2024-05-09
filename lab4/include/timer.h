#ifndef TIMER_H
#define TIMER_H

#include "dlist.h"

#define CORE0_TIMER_IRQCNTL ((volatile unsigned int *)(0x40000040))

typedef struct timer_task {
  double_linked_node_t node;
  unsigned long trigger_time;
  void *callback;
  char *callback_arg;
  int priority;
} timer_task_t;

void timer_list_init();
void core_timer_enable();
void core_timer_disable();
timer_task_t *create_timer_task(int secs, void *callback, const char *arg,
                                int priority);
void add_timer_task(timer_task_t *new_task);
void core_timer_handler();
void add_timer_task_to_irq_heap();
void core_timer_update();

#endif /* TIMER_H */