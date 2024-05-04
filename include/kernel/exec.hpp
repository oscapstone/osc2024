#pragma once

#include "mm/mm.hpp"

constexpr uint64_t USER_STACK_SIZE = PAGE_SIZE;

extern "C" {
// exec.S
void exec_user_prog(void* text, void* stack);
}

int exec(const char* name, char* const argv[]);
