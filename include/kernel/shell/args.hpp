#pragma once

#include "ds/stringarray.hpp"

struct Args : StringArray {
  Args(const char* buf, int len);
  Args(const char* const argv[]) : StringArray(argv) {};
  // TODO: copy constructor
  Args(const Args&) = delete;
};
