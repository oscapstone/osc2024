#pragma once

#include "util.hpp"

inline int get_el() {
  return read_sysreg(CurrentEL) >> 2;
}

extern "C" {
void exception_entry();
}
