#include "thread.hpp"

#include "io.hpp"
#include "sched.hpp"

Kthread::Kthread()
    : regs{},
      tid(new_tid()),
      status(KthreadStatus::kReady),
      exit_code(0),
      item(new KthreadItem(this)) {
  klog("init thread %d @ %p\n", tid, this);
}

Kthread::Kthread(Kthread::fp start)
    : regs{.x19 = (uint64_t)start, .lr = (void*)kthread_start},
      tid(new_tid()),
      status(KthreadStatus::kReady),
      exit_code(0),
      kernel_stack(KTHREAD_STACK_SIZE, true),
      item(new KthreadItem(this)) {
  klog("new thread %d @ %p\n", tid, this);
  reset_kernel_stack();
}

Kthread::Kthread(const Kthread& o)
    : regs(o.regs),
      tid(new_tid()),
      status(o.status),
      exit_code(0),
      kernel_stack(o.kernel_stack),
      user_text(o.user_text),
      user_stack(o.user_stack),
      item(new KthreadItem(this)) {
  fix(o, &regs, sizeof(regs));
  klog("fork thread %d @ %p from %d @ %p\n", tid, this, o.tid, &o);
}

void Kthread::fix(const Kthread& o, void* faddr, uint64_t fsize) {
  kernel_stack.fix(o.kernel_stack, faddr, fsize);
  user_text.fix(o.user_text, faddr, fsize);
  user_stack.fix(o.user_stack, faddr, fsize);
}

int Kthread::alloc_user_text_stack(uint64_t text_size, uint64_t stack_size) {
  user_text.dealloc();
  user_stack.dealloc();

  if (not user_text.alloc(text_size, false)) {
    klog("%s: can't alloc user_text for thread %d / size = %lx\n", __func__,
         tid, text_size);
    return -1;
  }

  if (not user_stack.alloc(stack_size, true)) {
    klog("%s: can't alloc user_stack / size = %lx\n", __func__, stack_size);
    return -1;
  }

  return 0;
}

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
  set_current_thread(thread);
}

void kthread_start() {
  Kthread::fp func;
  asm volatile("mov %0, x19" : "=r"(func));
  klog("start thread %d @ %p\n", current_thread()->tid, func);
  func();
  kthread_fini();
}

void kthread_exit(int status) {
  current_thread()->exit_code = status;
  kthread_fini();
}

void kthread_fini() {
  klog("fini thread %d\n", current_thread()->tid);
  current_thread()->status = KthreadStatus::kDead;
  schedule();
}

Kthread* kthread_create(Kthread::fp start) {
  auto thread = new Kthread(start);
  push_rq(thread);
  return thread;
}

long kthread_fork() {
  auto othread = current_thread();
  auto nthread = new Kthread(*othread);

  save_regs(&nthread->regs);

  if (current_thread()->tid == nthread->tid) {
    return 0;
  } else {
    nthread->fix(*othread, &nthread->regs, sizeof(Regs));
    push_rq(nthread);
    return nthread->tid;
  }
}
