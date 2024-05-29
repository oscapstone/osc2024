#include "signal.hpp"

#include "exec.hpp"
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

Signal::Signal(Kthread* thread) : cur(thread), list{}, stack{} {
  setall(signal_handler_nop);
  actions[SIGKILL] = {
      .in_kernel = true,
      .handler = signal_handler_kill,
  };
}

Signal::Signal(Kthread* thread, const Signal& other)
    : cur(thread), list{}, stack{} {
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
    klog("thread %d handle signal %d @ %p\n", cur->tid, sig, act.handler);

    if (act.in_kernel) {
      act.handler(sig);
    } else {
      if (frame) {
        if (not stack.alloc(PAGE_SIZE, true))
          panic("[signal::handle] can't alloc new stack");
        map_user_phy_pages(USER_SIGNAL_STACK, va2pa(stack.addr), PAGE_SIZE,
                           ProtFlags::RWX);
        auto backup_frame =
            (char*)USER_SIGNAL_STACK + PAGE_SIZE - sizeof(TrapFrame);
        memcpy(backup_frame, frame, sizeof(TrapFrame));
        frame->sp_el0 = (uint64_t)backup_frame;
        frame->elr_el1 = (uint64_t)act.handler;
        frame->X[0] = sig;
        frame->lr = (uint64_t)va2vsys(el0_sig_return);
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
  current_thread()->signal.stack.dealloc();
}

// run on EL0
void SECTION_VSYSCALL el0_sig_return() {
  asm volatile("svc 1\n");
}
