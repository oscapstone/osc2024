#pragma once

#include "pair.hpp"
#include "util.hpp"

constexpr uint64_t PAGE_SIZE = 0x1000;

template <typename T>
inline bool isPageAlign(T ptr) {
  return 0 == (uint64_t)ptr % PAGE_SIZE;
}

template <typename T,
          typename R = std::conditional_t<sizeof(T) == sizeof(void*), T, void*>>
inline R getPage(T ptr) {
  return (R)((uint64_t)ptr & (~(PAGE_SIZE - 1)));
}

void mm_reserve_p(void* start, void* end);
template <typename T, typename U>
void mm_reserve(T start, U end) {
  mm_reserve_p((void*)(uint64_t)start, (void*)(uint64_t)end);
}

pair<uint64_t, uint64_t> mm_range();
void mm_preinit();
void mm_init();

void* kmalloc(uint64_t size, uint64_t align = 1);
void* kcalloc(uint64_t size, uint64_t align = 1);
void kfree(void* ptr);
