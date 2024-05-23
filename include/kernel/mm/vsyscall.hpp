#pragma once

#include <type_traits>

#include "mm/mmu.hpp"

#define SECTION_VSYSCALL __attribute__((section(".vsyscall")))

extern char __vsyscall_beg[];
extern char __vsyscall_end[];
constexpr uint64_t VSYSCALL_START = 0xfffffffff000;

template <typename T,
          typename R = std::conditional_t<sizeof(T) == sizeof(void*), T, void*>>
inline R vsys2va(T x) {
  return (R)((uint64_t)x - VSYSCALL_START + __vsyscall_beg);
}
template <typename T,
          typename R = std::conditional_t<sizeof(T) == sizeof(void*), T, void*>>
inline R va2vsys(T x) {
  return (R)((uint64_t)x - (uint64_t)__vsyscall_beg + VSYSCALL_START);
}
