#pragma once

#include <cstdint>

struct timeval {
  uint32_t sec, usec;
};

#define PRTval   "%4d.%06d"
#define FTval(t) (t).sec, (t).usec
