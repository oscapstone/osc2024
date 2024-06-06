#include "thread.hpp"

#include "board/pm.hpp"
#include "int/interrupt.hpp"
#include "io.hpp"
#include "sched.hpp"
#include "syscall.hpp"

SYSCALL_DEFINE0(getpid) {
  return current_thread()->tid;
}

SYSCALL_DEFINE0(fork) {
  return kthread_fork();
}

SYSCALL_DEFINE1(exit, int, status) {
  kthread_exit(status);
  return -1;
}

SYSCALL_DEFINE1(kill, int, pid) {
  kthread_kill(pid);
  return 0;
}

ListHead<Kthread*> kthreads;

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
    : regs{}, tid(new_tid()), status(kReady), exit_code(0), signal{this} {
  signal.setall([](int) {
    klog("kill init cause reboot!\n");
    reboot();
  });
  klog("init thread %d @ %p\n", tid, this);
  add_list(this);
}

Kthread::Kthread(Kthread::fp start, void* ctx, Vnode* cwd)
    : regs{.x19 = (uint64_t)start,
           .x20 = (uint64_t)ctx,
           .lr = (void*)kthread_start},
      tid(new_tid()),
      status(kReady),
      exit_code(0),
      kernel_stack(KTHREAD_STACK_SIZE, true),
      signal{this},
      files{cwd} {
  klog("new thread %d @ %p\n", tid, this);
  reset_kernel_stack();
  add_list(this);
}

Kthread::Kthread(const Kthread* o)
    : regs(o->regs),
      tid(new_tid()),
      status(o->status),
      exit_code(0),
      kernel_stack(o->kernel_stack),
      signal{this, o->signal},
      vmm(o->vmm),
      files(o->files) {
  fix(*o, &regs, sizeof(regs));
  fix(*o, kernel_stack);
  klog("fork thread %d @ %p from %d @ %p\n", tid, this, o->tid, &o);
  add_list(this);
}

Kthread::~Kthread() {
  del_list(this);
}

void Kthread::fix(const Kthread& o, Mem& mem) {
  fix(o, mem.addr, mem.size);
}

void Kthread::fix(const Kthread& o, void* faddr, uint64_t fsize) {
  kernel_stack.fix(o.kernel_stack, faddr, fsize);
}

void* Kthread::fix(const Kthread& o, void* ptr) {
  ptr = kernel_stack.fix(o.kernel_stack, ptr);
  return ptr;
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
  Kthread* th;
  while ((th = find_thread_by_tid(pid)) and th->status != kDead)
    schedule();
}

void kthread_kill(int pid) {
  auto thread = find_thread_by_tid(pid);
  if (thread)
    kthread_kill(thread);
}

void kthread_kill(Kthread* thread) {
  if (thread == nullptr or thread->status == kDead)
    return;
  klog("kill thread %d\n", thread->tid);
  if (thread == current_thread()) {
    kthread_exit(-1);
  } else {
    save_DAIF_disable_interrupt();

    thread->exit_code = -1;
    thread->status = kDead;
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
  current_thread()->status = kDead;
  schedule();
}

Kthread* kthread_create(Kthread::fp start, void* ctx, Vnode* cwd) {
  auto thread = new Kthread{start, ctx, cwd};
  push_rq(thread);
  return thread;
}

long kthread_fork() {
  auto othread = current_thread();
  auto nthread = new Kthread(othread);

  save_regs(&nthread->regs);

  if (current_thread()->tid == nthread->tid) {
    return 0;
  } else {
    nthread->fix(*othread, &nthread->regs, sizeof(Regs));
    push_rq(nthread);
    return nthread->tid;
  }
}
