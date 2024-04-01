#include "task.h"

#include "alloc.h"
#include "uart1.h"

extern void enable_interrupt();
extern void disable_interrupt();

static task* t_head = (task*)0;
static unsigned int task_cnt = 0;

void add_task(task_callback cb, unsigned prio) {
  task* new_task = (task*)simple_malloc(sizeof(task));
  new_task->func = cb;
  new_task->next = (task*)0;
  new_task->priority = prio;

  disable_interrupt();

  task_cnt++;
  uart_send_string("add_task: task count: ");
  uart_int(task_cnt);
  uart_send_string("\r\n");

  if (!t_head || t_head->priority > new_task->priority) {
    new_task->next = t_head;
    t_head = new_task;
  } else {
    task* cur = t_head;
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
  while (t_head) {
    disable_interrupt();

    task_cnt--;
    uart_send_string("pop_task: task count: ");
    uart_int(task_cnt);
    uart_send_string("\r\n");

    t_head->func();
    t_head = t_head->next;
    enable_interrupt();
  }
}