#pragma once

#include "exception.hpp"
#include "list.hpp"

extern "C" {
void irq_handler(ExceptionContext* context, int type);
}

struct Task : ListItem {
  int prio;  // high priority task = high number

  using fp = void (*)(void*);
  fp callback;
  void* context;

  Task(int prio_, fp callback_, void* context_)
      : ListItem{}, prio{prio_}, callback{callback_}, context{context_} {}

  void call() const {
    return callback(context);
  }
};

extern ListHead<Task> irq_tasks;

void irq_add_task(int prio, Task::fp callback, void* context);
void irq_run();
