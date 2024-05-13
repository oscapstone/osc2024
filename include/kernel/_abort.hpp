#pragma once

#include "io.hpp"
#include "util.hpp"

namespace std {
inline void abort() {
  panic("abort");
}
};  // namespace std
