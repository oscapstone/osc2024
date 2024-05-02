#include "syscall.hpp"

#include "io.hpp"
#include "thread.hpp"

void syscall_handler(TrapFrame* frame) {
  uint64_t syscallno = frame->X[8];
  uint64_t ret;
  klog("do syscall %ld\n", syscallno);
  if (syscallno < __NR_syscalls) {
    ret = sys_call_table[syscallno](frame);
  } else {
    ret = -1;
  }
  frame->X[0] = ret;
}

#undef __SYSCALL
#define __SYSCALL(nr, sym) [nr] = sym,
const syscall_fn_t sys_call_table[__NR_syscalls] = {
#include "unistd.hpp"
};

long sys_getpid(const TrapFrame* /*frame*/) {
  return current_thread()->tid;
}
