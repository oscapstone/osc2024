#pragma once

#include "io.hpp"
#include "util.hpp"

namespace std {
inline void abort() {
  kputs_sync("abort");
  prog_hang();
}
};  // namespace std
