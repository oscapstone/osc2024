#include "syscall.hpp"

#include "exec.hpp"
#include "int/interrupt.hpp"
#include "io.hpp"
#include "thread.hpp"
#include "util.hpp"

void syscall_handler(TrapFrame* frame) {
  uint64_t syscallno = frame->X[8];
  uint64_t ret;
  // klog("do syscall %ld\n", syscallno);
  if (syscallno < __NR_syscalls) {
    enable_interrupt();
    ret = sys_call_table[syscallno](frame);
    disable_interrupt();
  } else {
    ret = -1;
  }
  frame->X[0] = ret;
}

#undef __SYSCALL
#define __SYSCALL(nr, sym) sym,
const syscall_fn_t sys_call_table[__NR_syscalls] = {
#include "unistd.hpp"
};

long sys_getpid(const TrapFrame* /*frame*/) {
  return current_thread()->tid;
}

long sys_uartread(const TrapFrame* frame) {
  auto buf = (char*)frame->X[0];
  auto size = (unsigned)frame->X[1];
  return kread(buf, size);
}

long sys_uartwrite(const TrapFrame* frame) {
  auto buf = (const char*)frame->X[0];
  auto size = (unsigned)frame->X[1];
  return kwrite(buf, size);
}

long sys_exec(const TrapFrame* frame) {
  auto name = (const char*)frame->X[0];
  auto argv = (char** const)frame->X[1];
  return exec(name, argv);
}

long sys_fork(const TrapFrame* frame) {
  return kthread_fork();
}

long sys_exit(const TrapFrame* frame) {
  auto status = (int)frame->X[0];
  kthread_exit(status);
  return -1;
}

long sys_not_implement(const TrapFrame* /*frame*/) {
  klog("syscall not implemented\n");
  return -1;
}

STRONG_ALIAS(sys_not_implement, sys_mbox_call);
STRONG_ALIAS(sys_not_implement, sys_kill);
STRONG_ALIAS(sys_not_implement, sys_signal);
STRONG_ALIAS(sys_not_implement, sys_signal_kill);
