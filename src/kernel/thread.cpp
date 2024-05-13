#include "thread.hpp"

#include "board/pm.hpp"
#include "int/interrupt.hpp"
#include "int/timer.hpp"
#include "io.hpp"
#include "sched.hpp"

ListHead<Kthread> kthreads;

void add_list(Kthread* thread) {
  kthreads.push_back(thread);
}
void del_list(Kthread* thread) {
  kthreads.erase(thread);
}

// TODO: don't use linear search
Kthread* find_thread_by_tid(int tid) {
  for (auto thread : kthreads)
    if (thread->tid == tid)
      return thread;
  return nullptr;
}

Kthread::Kthread()
    : regs{},
      tid(new_tid()),
      status(KthreadStatus::kReady),
      exit_code(0),
      item(new KthreadItem(this)),
      signal{this} {
  signal.setall([](int) {
    klog("kill init cause reboot!\n");
    reboot();
  });
  klog("init thread %d @ %p\n", tid, this);
  add_list(this);
}

Kthread::Kthread(Kthread::fp start, void* ctx)
    : regs{.x19 = (uint64_t)start,
           .x20 = (uint64_t)ctx,
           .lr = (void*)kthread_start},
      tid(new_tid()),
      status(KthreadStatus::kReady),
      exit_code(0),
      kernel_stack(KTHREAD_STACK_SIZE, true),
      item(new KthreadItem(this)),
      signal{this} {
  klog("new thread %d @ %p\n", tid, this);
  reset_kernel_stack();
  add_list(this);
}

Kthread::Kthread(const Kthread& o)
    : regs(o.regs),
      tid(new_tid()),
      status(o.status),
      exit_code(0),
      kernel_stack(o.kernel_stack),
      user_text(o.user_text),
      user_stack(o.user_stack),
      item(new KthreadItem(this)),
      signal{this, o.signal} {
  fix(o, &regs, sizeof(regs));
  fix(o, kernel_stack);
  fix(o, user_stack);
  klog("fork thread %d @ %p from %d @ %p\n", tid, this, o.tid, &o);
  add_list(this);
}

Kthread::~Kthread() {
  del_list(this);
  delete item;
}

void Kthread::fix(const Kthread& o, Mem& mem) {
  fix(o, mem.addr, mem.size);
}

void Kthread::fix(const Kthread& o, void* faddr, uint64_t fsize) {
  kernel_stack.fix(o.kernel_stack, faddr, fsize);
  user_text.fix(o.user_text, faddr, fsize);
  user_stack.fix(o.user_stack, faddr, fsize);
}

void* Kthread::fix(const Kthread& o, void* ptr) {
  ptr = kernel_stack.fix(o.kernel_stack, ptr);
  ptr = user_text.fix(o.user_text, ptr);
  ptr = user_stack.fix(o.user_stack, ptr);
  return ptr;
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
  kthreads.init();
  extern char __stack_beg[], __stack_end[];
  auto thread = new Kthread;
  thread->kernel_stack.addr = __stack_beg;
  thread->kernel_stack.size = __stack_end - __stack_beg;
  set_current_thread(thread);
}

void kthread_start() {
  enable_interrupt();
  Kthread::fp func;
  void* ctx;
  asm volatile("mov %0, x19" : "=r"(func));
  asm volatile("mov %0, x20" : "=r"(ctx));
  klog("start thread %d @ %p\n", current_thread()->tid, func);
  func(ctx);
  kthread_fini();
}

// TODO: wq
void kthread_wait(int pid) {
  while (find_thread_by_tid(pid))
    schedule();
}

void kthread_kill(int pid) {
  auto thread = find_thread_by_tid(pid);
  if (thread)
    kthread_kill(thread);
}

void kthread_kill(Kthread* thread) {
  if (thread == nullptr or thread->status == KthreadStatus::kDead)
    return;
  klog("kill thread %d\n", thread->tid);
  if (thread == current_thread()) {
    kthread_exit(-1);
  } else {
    save_DAIF_disable_interrupt();

    thread->exit_code = -1;
    thread->status = KthreadStatus::kDead;
    erase_rq(thread);
    push_dead(thread);

    restore_DAIF();
  }
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

Kthread* kthread_create(Kthread::fp start, void* ctx) {
  auto thread = new Kthread(start, ctx);
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
