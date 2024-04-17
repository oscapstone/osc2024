#include "task.h"

#include "alloc.h"
#include "interrupt.h"
#include "uart1.h"

static uint8_t cur_priority = NO_TASK;
static handler_task_t* t_head = (handler_task_t*)0;

void add_task(task_callback cb, uint32_t prio) {
  handler_task_t* new_task =
      (handler_task_t*)simple_malloc(sizeof(handler_task_t));
  if (!new_task) {
    uart_puts("add_task: new_task memory allocation fail");
    return;
  }
  new_task->func = cb;
  new_task->next = (handler_task_t*)0;
  new_task->priority = prio;

  disable_interrupt();
  if (!t_head || t_head->priority > new_task->priority) {
    new_task->next = t_head;
    t_head = new_task;
  } else {
    handler_task_t* cur = t_head;
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
  uint32_t ori_priority = cur_priority;
  while (t_head && t_head->priority != ori_priority) {
    disable_interrupt();
    cur_priority = t_head->priority;
    enable_interrupt();

    t_head->func();

    disable_interrupt();
    t_head = t_head->next;
    enable_interrupt();
  }
  cur_priority = ori_priority;
}