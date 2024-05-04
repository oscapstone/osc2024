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
  thread->item = new KthreadItem(thread);
  set_current_thread(thread);
}

void kthread_start() {
  Kthread::fp func;
  asm volatile("mov %0, x19" : "=r"(func));
  klog("start thread %d @ %p\n", current_thread()->tid, func);
  func();
  kthread_fini();
}

void kthread_fini() {
  klog("fini thread %d\n", current_thread()->tid);
  current_thread()->status = KthreadStatus::kDead;
  schedule();
}

void Kthread::init(Kthread::fp start) {
  tid = new_tid();
  if (kernel_stack)
    kfree(kernel_stack);
  status = KthreadStatus::kReady;
  exit_code = 0;
  kernel_stack = (char*)kmalloc(KTHREAD_STACK_SIZE);
  regs.init();
  regs.lr = (void*)kthread_start;
  regs.x19 = (uint64_t)start;
  regs.sp = kernel_stack + KTHREAD_STACK_SIZE - 0x10;
  item = new KthreadItem(this);
}

Kthread* kthread_create(Kthread::fp start) {
  auto thread = new Kthread;
  thread->init(start);
  push_rq(thread);
  return thread;
}

int Kthread::alloc_user_text_stack(uint64_t text_size, uint64_t stack_size) {
  kfree(user_text);
  kfree(user_stack);

  user_text = (char*)kmalloc(text_size, PAGE_SIZE);
  if (user_text == nullptr) {
    klog("%s: can't alloc user_text for thread %d / size = %lx\n", __func__,
         tid, text_size);
    return -1;
  }

  user_stack = (char*)kmalloc(stack_size, PAGE_SIZE);
  if (user_stack == nullptr) {
    klog("%s: can't alloc user_stack / size = %lx\n", __func__, stack_size);
    return -1;
  }

  return 0;
}
