#include "syscall.hpp"

#include "int/interrupt.hpp"
#include "io.hpp"
#include "util.hpp"

void syscall_handler(TrapFrame* frame) {
  uint64_t syscallno = frame->X[8];
  uint64_t ret;
  // klog("do syscall %ld\n", syscallno);
  if (syscallno < NR_syscalls) {
    enable_interrupt();
    ret = sys_call_table[syscallno](frame);
    disable_interrupt();
  } else {
    ret = -1;
  }
  frame->X[0] = ret;
}

long sys_null(const TrapFrame* /* frame */) {
  return 0;
}

long sys_not_implement(const TrapFrame* frame) {
  uint64_t syscallno = frame->X[8];
  klog("syscall %ld not implemented\n", syscallno);
  return -1;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc99-designator"
#pragma GCC diagnostic ignored "-Winitializer-overrides"
#undef __SYSCALL
#define __SYSCALL(nr, sym) [nr] = sym,
const syscall_fn_t sys_call_table[NR_syscalls] = {
    [0 ... NR_syscalls - 1] = sys_null,
#include "unistd.hpp"
};
#pragma GCC diagnostic pop
