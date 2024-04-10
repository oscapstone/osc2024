#pragma once

#include "util.hpp"

struct ExceptionContext {
  uint64_t X[30];
  uint64_t lr;
  uint64_t spsr_el1;
  uint64_t elr_el1;
  uint64_t esr_el1;
};

inline int get_el() {
  return read_sysreg(CurrentEL) >> 2;
}

extern "C" {
void exception_handler(ExceptionContext*, int type);
}
