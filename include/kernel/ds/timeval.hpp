#pragma once

#include <cstdint>

#include "string.hpp"

struct timeval {
  uint32_t sec, usec;
};

#define PRTval   "%4d.%06d"
#define FTval(t) (t).sec, (t).usec

inline timeval parseTval(const char* s) {
  uint32_t sec = strtol(s);
  uint32_t usec = 0;
  auto p = strchr(s, '.');
  if (*p) {
    p++;
    for (int i = 0; i < 6; i++) {
      auto c = *p;
      auto in = '0' <= c and c <= '9';
      usec = usec * 10 + (in ? c - '0' : 0);
      if (in)
        p++;
    }
  }
  return {sec, usec};
}
