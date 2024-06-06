#pragma once

#include "ds/list.hpp"
#include "int/exception.hpp"

struct Task : ListItem<Task> {
  bool running;
  int prio;  // high priority task = high number

  using fp = void (*)(void*);
  fp callback;
  void* context;
  using fini_fp = void (*)();
  fini_fp fini;

  Task()
      : ListItem{},
        running{false},
        prio{-1},
        callback{nullptr},
        context{nullptr},
        fini{nullptr} {}

  Task(int prio_, fp callback_, void* context_ = nullptr,
       fini_fp fini_ = nullptr)
      : ListItem{},
        running{false},
        prio{prio_},
        callback{callback_},
        context{context_},
        fini{fini_} {}

  void call() const {
    if (callback)
      callback(context);
  }
  void call_fini() const {
    if (fini)
      fini();
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

extern ListHead<Task*> irq_tasks;

void irq_init();
void irq_add_task(int prio, Task::fp callback, void* context = nullptr,
                  Task::fini_fp = nullptr);
void irq_run();
