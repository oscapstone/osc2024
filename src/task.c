#include "task.h"

#include "alloc.h"
#include "uart1.h"

extern void enable_interrupt();
extern void disable_interrupt();

static task* t_head = (task*)0;

void add_task(task_callback cb, uint32_t prio) {
  task* new_task = (task*)simple_malloc(sizeof(task));
  if (!new_task) {
    uart_puts("add_task: new_task memory allocation fail");
    return;
  }
  new_task->func = cb;
  new_task->next = (task*)0;
  new_task->priority = prio;
  if (!t_head || t_head->priority > new_task->priority) {
    new_task->next = t_head;
    t_head = new_task;
  } else {
    task* cur = t_head;
    while (1) {
      if (!cur->next || cur->next->priority >= new_task->priority) {
        new_task->next = cur->next;
        cur->next = new_task;
        break;
      }
      cur = cur->next;
    }
  }
  enable_interrupt();
}

void pop_task() {
  task* h_t;
  while (t_head) {
    disable_interrupt();
    h_t = t_head;
    t_head = t_head->next;
    enable_interrupt();

    h_t->func();
  }
}