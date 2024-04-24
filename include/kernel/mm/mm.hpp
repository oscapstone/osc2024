#pragma once

#include <cstdint>

constexpr uint64_t PAGE_SIZE = 0x1000;

inline bool isPageAlign(void* ptr) {
  return 0 == (uint64_t)ptr % PAGE_SIZE;
}
inline void* getPage(void* ptr) {
  return (void*)((uint64_t)ptr & (~(PAGE_SIZE - 1)));
}

void mm_init();

void* kmalloc(uint64_t size, uint64_t align = 1);
void kfree(void* ptr);
