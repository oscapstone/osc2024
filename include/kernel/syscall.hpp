#pragma once

#include "int/exception.hpp"
#include "unistd.hpp"

void syscall_handler(TrapFrame* frame);

using syscall_fn_t = long (*)(const TrapFrame*);
extern const syscall_fn_t sys_call_table[__NR_syscalls];

#undef __SYSCALL
#define __SYSCALL(nr, sym) long sym(const TrapFrame*);
#include "unistd.hpp"
