#include "task.h"

#include "alloc.h"

static task* t_head = (task*)0;

void add_task(task_callback cb, unsigned prio) {
  task* new_task = (task*)simple_malloc(sizeof(task));
  new_task->func = cb;
  new_task->next = (task*)0;
  new_task->priority = prio;


  if (!t_head || t_head->priority > new_task->priority) {
    new_task->next = t_head;
    t_head = new_task;
  } else {
    task* cur = t_head;
    while (1) {
      if (!cur->next || cur->next->priority > new_task->priority) {
        new_task->next = cur->next;
        cur->next = new_task;
      }
      cur = cur->next;
    }
  }
}