#pragma once

#include <cstdint>

constexpr uint64_t PAGE_SIZE = 0x1000;

inline bool isPageAlign(void* ptr) {
  return 0 == (uint64_t)ptr % PAGE_SIZE;
}
inline void* getPage(void* ptr) {
  return (void*)((uint64_t)ptr & (~(PAGE_SIZE - 1)));
}

#define MM_LOG_LEVEL 3

#if MM_LOG_LEVEL >= 3
#define MM_DEBUG_PRINT(fmt, ...) klog(fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define MM_DEBUG_PRINT(fmt, ...) 0
#endif

#define MM_DEBUG(type, fmt, ...) \
  MM_DEBUG_PRINT("[" type "] [DEBUG] " fmt __VA_OPT__(, ) __VA_ARGS__)

#if MM_LOG_LEVEL >= 2
#define MM_INFO_PRINT(fmt, ...) klog(fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define MM_INFO_PRINT(fmt, ...) 0
#endif

#define MM_INFO(type, fmt, ...) \
  MM_INFO_PRINT("[" type "] [*] " fmt __VA_OPT__(, ) __VA_ARGS__)

void mm_init();
