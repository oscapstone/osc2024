#include "task.h"

#include "interrupt.h"
#include "mem.h"
#include "uart1.h"

static uint8_t cur_priority = NO_TASK;
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

  OS_enter_critical();
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
  OS_exit_critical();
}

void pop_task() {
  uint32_t ori_priority = cur_priority;
  while (t_head && t_head->priority != ori_priority) {
    OS_enter_critical();
    cur_priority = t_head->priority;
    OS_exit_critical();

    t_head->func();

    OS_enter_critical();
    handler_task_t* next = t_head->next;
    free(t_head);
    t_head = next;
    OS_exit_critical();
  }
  cur_priority = ori_priority;
}