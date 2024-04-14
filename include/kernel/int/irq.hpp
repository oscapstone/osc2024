#pragma once

#include "ds/list.hpp"
#include "int/exception.hpp"

extern "C" {
void irq_handler(ExceptionContext* context, int type);
}

struct Task : ListItem {
  bool running;
  int prio;  // high priority task = high number

  using fp = void (*)(void*);
  fp callback;
  void* context;

  Task()
      : ListItem{},
        running{false},
        prio{-1},
        callback{nullptr},
        context{nullptr} {}

  Task(int prio_, fp callback_, void* context_)
      : ListItem{},
        running{false},
        prio{prio_},
        callback{callback_},
        context{context_} {}

  void call() const {
    return callback(context);
  }
};

inline void link(Task* prev, Task* next) {
  if (prev)
    prev->next = next;
  if (next)
    next->prev = prev;
}
inline void unlink(Task* it) {
  link(it->prev, it->next);
}

extern ListHead<Task> irq_tasks;

void irq_init();
void irq_add_task(int prio, Task::fp callback, void* context);
void irq_run();
