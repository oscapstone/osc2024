#pragma once

#include "mm/mm.hpp"
#include "shell/args.hpp"
#include "string.hpp"
#include "util.hpp"

constexpr uint64_t USER_STACK_SIZE = PAGE_SIZE;

extern "C" {
// exec.S
void exec_user_prog(void* user_text, void* user_stack, void* kernel_stack);
}

struct ExecCtx {
  const char* name;
  Args args;
  // TODO: envp
  ExecCtx(const char* name, const char* const argv[])
      : name(strdup(name)), args(argv) {}
};

int exec(ExecCtx* ctx);
void exec_new_user_prog(void* ctx);
