#include "sched.hpp"

#include "int/interrupt.hpp"
#include "io.hpp"

ListHead<ThreadItem> rq;
void push_rq(Thread* thread) {
  rq.push_back(thread->item);
}
Thread* pop_rq() {
  return rq.pop_front()->thread;
}

int new_tid() {
  static int tid = 0;
  return tid++;
}

void run_thread() {
  Thread::fp func;
  asm volatile("mov %0, x19" : "=r"(func));
  klog("start thread %p\n", func);
  enable_interrupt();
  func();
  current_thread()->dead = true;
  schedule();
}

void Thread::init(Thread::fp start) {
  tid = new_tid();
  stack = (char*)kmalloc(STACK_SIZE);
  regs.init();
  regs.lr = (void*)run_thread;
  regs.x19 = (uint64_t)start;
  regs.sp = stack + STACK_SIZE - 0x10;
  item = new ThreadItem(this);
}

Thread* create_thread(Thread::fp start) {
  auto thread = new Thread;
  thread->init(start);
  push_rq(thread);
  return thread;
}

void idle() {
  while (true) {
    kill_zombies();
    schedule();
  }
}

void init_thread() {
  auto thread = new Thread;
  thread->tid = new_tid();
  thread->item = new ThreadItem(thread);
  set_current_thread(thread);
}

void kill_zombies() {
  // TODO
}

void schedule_init() {
  rq.init();
  init_thread();
}

void schedule() {
  disable_interrupt();

  auto cur = current_thread();
  if (not cur->dead)
    push_rq(cur);

  auto nxt = pop_rq();
  switch_to(cur, nxt);

  enable_interrupt();
}
