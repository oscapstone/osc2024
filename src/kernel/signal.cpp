#include "signal.hpp"

#include "thread.hpp"

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
    klog("thread %d handle signal %d @ %p\n", cur->tid, sig, act.handler);

    if (act.in_kernel) {
      act.handler(sig);
    } else {
      auto new_stack = (char*)kmalloc(PAGE_SIZE);
      char* new_stack_end = new_stack + PAGE_SIZE - sizeof(TrapFrame);
      memcpy(new_stack_end, frame, sizeof(TrapFrame));
      frame->sp_el0 = (uint64_t)new_stack_end;
      frame->elr_el1 = (uint64_t)act.handler;
      frame->X[0] = sig;
      frame->lr = (uint64_t)el0_sig_return;
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
  auto new_stack = (char*)getPage((void*)frame->sp_el0);
  char* new_stack_end = new_stack + PAGE_SIZE - sizeof(TrapFrame);
  memcpy(frame, new_stack_end, sizeof(TrapFrame));
}

// run on EL0
void el0_sig_return() {
  asm volatile("svc 1\n");
}
