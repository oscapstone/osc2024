#include "signal.hpp"

#include "thread.hpp"

Signal::Signal(Kthread* thread) : cur(thread), list{} {
  for (int i = 0; i < NSIG; i++) {
    actions[i] = {
        .in_kernel = true,
        .handler = signal_handler_nop,
    };
  }
  actions[SIGKILL] = {
      .in_kernel = true,
      .handler = signal_handler_kill,
  };
}

Signal::Signal(Kthread* thread, const Signal& other) : cur(thread), list{} {
  for (int i = 0; i < NSIG; i++)
    actions[i] = other.actions[i];
}

void Signal::regist(int signal, signal_handler handler) {
  if (signal >= NSIG)
    return;
  actions[signal] = {
      .in_kernel = false,
      .handler = handler,
  };
}

void Signal::operator()(int signal) {
  if (signal >= NSIG)
    return;
  klog("thread %d signal %d\n", cur->tid, signal);
  list.push_back(new SignalItem{signal});
}

void Signal::handle(TrapFrame* frame) {
  while (not list.empty()) {
    auto it = list.pop_front();
    auto sig = it->signal;
    delete it;

    auto& act = actions[sig];
    klog("thread %d handle signal %d @ %p\n", cur->tid, sig, act.handler);

    if (act.in_kernel) {
      act.handler();
    } else {
      auto new_stack = (char*)kmalloc(PAGE_SIZE);
      char* new_stack_end = new_stack + PAGE_SIZE - sizeof(TrapFrame);
      memcpy(new_stack_end, frame, sizeof(TrapFrame));
      frame->sp_el0 = (uint64_t)new_stack_end;
      frame->elr_el1 = (uint64_t)act.handler;
      frame->lr = (uint64_t)el0_sig_return;
      break;
    }
  }
}

void signal_handler_nop() {}

void signal_handler_kill() {
  kthread_exit(-1);
}

void signal_kill(int pid, int signal) {
  auto thread = find_thread_by_tid(pid);
  if (not thread)
    return;
  thread->signal(signal);
}

void signal_return(TrapFrame* frame) {
  auto new_stack = (char*)getPage((void*)frame->sp_el0);
  char* new_stack_end = new_stack + PAGE_SIZE - sizeof(TrapFrame);
  memcpy(frame, new_stack_end, sizeof(TrapFrame));
}

// run on EL0
void el0_sig_return() {
  asm volatile(
      "mov x8, 10\n"
      "svc 0\n");
}
