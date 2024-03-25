#pragma once

#include <cstdint>

uint64_t reloc_impl(uint64_t);
extern uint64_t reloc_base;

template <typename T>
T reloc(T x) {
  return (T)reloc_impl((uint64_t)x);
}
