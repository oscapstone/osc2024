#pragma once

#include "int/exception.hpp"
#include "unistd.hpp"

void syscall_handler(TrapFrame* frame);

using syscall_fn_t = long (*)(const TrapFrame*);
extern const syscall_fn_t sys_call_table[NR_syscalls];

extern "C" {
#undef __SYSCALL
#define __SYSCALL(nr, sym) long sym(const TrapFrame*);
#include "unistd.hpp"
long sys_not_implement(const TrapFrame* frame);
}

// ref:
// https://github.com/torvalds/linux/blob/v6.9/include/linux/syscalls.h
// https://github.com/torvalds/linux/blob/v6.9/arch/arm64/include/asm/syscall_wrapper.h

#define __MAP0(m, ...)
#define __MAP1(m, t, a, ...) m(t, a)
#define __MAP2(m, t, a, ...) m(t, a), __MAP1(m, __VA_ARGS__)
#define __MAP(n, ...)        __MAP##n(__VA_ARGS__)

#define __SC_DECL(t, a) t a
#define __SC_LONG(t, a) long a
#define __SC_CAST(t, a) (t) a
#define __SC_TYPE(t, a) t
#define __SC_ARGS(t, a) a

#define REGS_TO_ARGS(x, ...)                                         \
  __MAP(x, __SC_ARGS, , frame->X[0], , frame->X[1], , frame->X[2], , \
        frame->X[3], , frame->X[4], , frame->X[5])

#define SYSCALL_DEFINEx(x, name, ...)                                    \
  static inline long __se_sys_##name(__MAP(x, __SC_LONG, __VA_ARGS__));  \
  static inline long __do_sys_##name(__MAP(x, __SC_DECL, __VA_ARGS__));  \
  long sys_##name(const TrapFrame* frame) {                              \
    return __se_sys_##name(REGS_TO_ARGS(x, __VA_ARGS__));                \
  }                                                                      \
  static inline long __se_sys_##name(__MAP(x, __SC_LONG, __VA_ARGS__)) { \
    return __do_sys_##name(__MAP(x, __SC_CAST, __VA_ARGS__));            \
  }                                                                      \
  static inline long __do_sys_##name(__MAP(x, __SC_DECL, __VA_ARGS__))

#define SYSCALL_DEFINE0(name) long sys_##name(const TrapFrame* /*frame*/)

#define SYSCALL_DEFINE1(name, ...) SYSCALL_DEFINEx(1, name, __VA_ARGS__)
#define SYSCALL_DEFINE2(name, ...) SYSCALL_DEFINEx(2, name, __VA_ARGS__)
