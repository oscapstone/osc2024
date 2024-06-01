#include "signal.hpp"

#include "exec.hpp"
#include "mm/vmm.hpp"
#include "mm/vsyscall.hpp"
#include "syscall.hpp"
#include "thread.hpp"

SYSCALL_DEFINE2(signal, int, signal, signal_handler, handler) {
  current_thread()->signal.regist(signal, handler);
  return 0;
}

SYSCALL_DEFINE2(signal_kill, int, pid, int, signal) {
  signal_kill(pid, signal);
  return 0;
}

Signal::Signal(Kthread* thread) : cur(thread), list{} {
  setall(signal_handler_nop);
  actions[SIGKILL] = {
      .in_kernel = true,
      .handler = signal_handler_kill,
  };
}

Signal::Signal(Kthread* thread, const Signal& other) : cur(thread), list{} {
  for (int i = 0; i < NSIG; i++)
    actions[i] = other.actions[i];
}

void Signal::regist(int sig, signal_handler handler) {
  if (sig >= NSIG)
    return;
  actions[sig] = {
      .in_kernel = false,
      .handler = handler,
  };
}

void Signal::setall(signal_handler handler) {
  for (int i = 0; i < NSIG; i++) {
    actions[i] = {
        .in_kernel = true,
        .handler = handler,
    };
  }
}

void Signal::operator()(int sig) {
  if (sig >= NSIG)
    return;
  klog("thread %d signal %d\n", cur->tid, sig);
  list.push_back(new SignalItem{sig});
}

void Signal::handle(TrapFrame* frame) {
  while (not list.empty()) {
    auto it = list.pop_front();
    auto sig = it->signal;
    delete it;

    auto& act = actions[sig];

    if (act.in_kernel) {
      klog("t%d handle signal %d @ kernel %p\n", cur->tid, sig, act.handler);
      act.handler(sig);
    } else {
      if (frame and is_invlid_addr(stack_addr)) {
        stack_addr = mmap(USER_SIGNAL_STACK, PAGE_SIZE, ProtFlags::RWX,
                          MmapFlags::NONE, "[signal_stack]");
        auto backup_frame = (char*)stack_addr + PAGE_SIZE - sizeof(TrapFrame);
        memcpy(backup_frame, frame, sizeof(TrapFrame));
        frame->sp_el0 = (uint64_t)backup_frame;
        frame->elr_el1 = (uint64_t)act.handler;
        frame->X[0] = sig;
        frame->lr = (uint64_t)va2vsys(el0_sig_return);
        klog("t%d handle signal %d @ user %p\n", cur->tid, sig, act.handler);
      } else {
        list.push_front(new SignalItem{sig});
      }
      break;
    }
  }
}

void signal_handler_nop(int) {}

void signal_handler_kill(int) {
  kthread_exit(-1);
}

void signal_kill(int pid, int sig) {
  auto thread = find_thread_by_tid(pid);
  if (not thread)
    return;
  thread->signal(sig);
}

void signal_return(TrapFrame* frame) {
  klog("thread %d signal_return\n", current_thread()->tid);
  auto backup_frame = (void*)frame->sp_el0;
  memcpy(frame, backup_frame, sizeof(TrapFrame));
  auto& stack_addr = current_thread()->signal.stack_addr;
  munmap(stack_addr, PAGE_SIZE);
  stack_addr = INVALID_ADDRESS;
}

// run on EL0
void SECTION_VSYSCALL el0_sig_return() {
  asm volatile("svc 1\n");
}
