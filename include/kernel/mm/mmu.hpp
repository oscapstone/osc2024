#pragma once

#include <type_traits>

#include "util.hpp"

constexpr uint64_t ADDRESS_SPACE_TAG = 0xFFFF000000000000;
constexpr uint64_t KERNEL_SPACE = 0xFFFF000000000000;
constexpr uint64_t USER_SPACE = 0;

template <typename T,
          typename R = std::conditional_t<sizeof(T) == sizeof(void*), T, void*>>
inline R va2pa(T x) {
  return (R)((uint64_t)x - KERNEL_SPACE);
}
template <typename T,
          typename R = std::conditional_t<sizeof(T) == sizeof(void*), T, void*>>
inline R pa2va(T x) {
  return (R)((uint64_t)x + KERNEL_SPACE);
}

template <typename T>
inline bool isKernelSpace(T x) {
  return ((uint64_t)x & ADDRESS_SPACE_TAG) == KERNEL_SPACE;
}
template <typename T>
inline bool isUserSpace(T x) {
  return ((uint64_t)x & ADDRESS_SPACE_TAG) == USER_SPACE;
}
