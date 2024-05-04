#include "thread.hpp"

#include "io.hpp"
#include "sched.hpp"

void idle() {
  while (true) {
    kill_zombies();
    schedule();
  }
}

int new_tid() {
  static int tid = 0;
  return tid++;
}

void kthread_init() {
  auto thread = new Kthread;
  thread->tid = new_tid();
  thread->item = new ThreadItem(thread);
  set_current_thread(thread);
}

void kthread_start() {
  Kthread::fp func;
  asm volatile("mov %0, x19" : "=r"(func));
  klog("start thread %p\n", func);
  func();
  kthread_fini();
}

void kthread_fini() {
  current_thread()->dead = true;
  schedule();
}

void Kthread::init(Kthread::fp start) {
  tid = new_tid();
  stack = (char*)kmalloc(KTHREAD_STACK_SIZE);
  regs.init();
  regs.lr = (void*)kthread_start;
  regs.x19 = (uint64_t)start;
  regs.sp = stack + KTHREAD_STACK_SIZE - 0x10;
  item = new ThreadItem(this);
}

Kthread* kthread_create(Kthread::fp start) {
  auto thread = new Kthread;
  thread->init(start);
  push_rq(thread);
  return thread;
}
