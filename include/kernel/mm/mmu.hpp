#pragma once

#include "util.hpp"

constexpr uint64_t ADDRESS_SPACE_TAG = 0xFFFF000000000000;
constexpr uint64_t KERNEL_SPACE = 0xFFFF000000000000;
constexpr uint64_t USER_SPACE = 0;

template <typename T>
inline T va2pa(T x) {
  return (T)((uint64_t)x - KERNEL_SPACE);
}
template <typename T>
inline T pa2va(T x) {
  return (T)((uint64_t)x + KERNEL_SPACE);
}

template <typename T>
inline bool isKernelSpace(T x) {
  return ((uint64_t)x & ADDRESS_SPACE_TAG) == KERNEL_SPACE;
}
template <typename T>
inline bool isUserSpace(T x) {
  return ((uint64_t)x & ADDRESS_SPACE_TAG) == USER_SPACE;
}
