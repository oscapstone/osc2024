#include "task.h"

#include "interrupt.h"
#include "mem.h"
#include "uart1.h"

static handler_task_t* t_head = (handler_task_t*)0;

void add_task(task_callback cb, uint32_t prio) {
  handler_task_t* new_task = (handler_task_t*)malloc(sizeof(handler_task_t));
  if (!new_task) {
    uart_puts("add_task: new_task memory allocation fail");
    return;
  }

  new_task->func = cb;
  new_task->next = (handler_task_t*)0;
  new_task->priority = prio;
  if (!t_head || t_head->priority > new_task->priority) {
    new_task->next = t_head;
    t_head = new_task;
  } else {
    handler_task_t* cur = t_head;
    while (1) {
      if (!cur->next || cur->next->priority > new_task->priority) {
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
  handler_task_t* h_t;
  while (t_head) {
    OS_enter_critical();
    h_t = t_head;
    t_head = t_head->next;
    OS_exit_critical();

    h_t->func();
    free(h_t);
  }
}