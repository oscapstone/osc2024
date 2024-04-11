#pragma once

#include "exception.hpp"
#include "list.hpp"

extern "C" {
void irq_handler(ExceptionContext* context, int type);
}

struct Task {
  Task *prev, *next;
  bool running;
  int prio;  // high priority task = high number

  using fp = void (*)(void*);
  fp callback;
  void* context;

  Task(int prio_, fp callback_, void* context_)
      : prev{nullptr},
        next{nullptr},
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

struct TaskHead {
  Task head{0, nullptr, nullptr}, tail{0, nullptr, nullptr};
  int count;
  void insert(Task* it, Task* node) {
    count++;
    link(it->prev, node);
    link(node, it);
  }
  void erase(Task* node) {
    count--;
    unlink(node);
  }
  bool empty() const {
    return count == 0;
  }
  auto begin() {
    return head.next;
  }
  auto end() {
    return &tail;
  }
};

extern TaskHead irq_tasks;

void irq_init();
void irq_add_task(int prio, Task::fp callback, void* context);
void irq_run();
