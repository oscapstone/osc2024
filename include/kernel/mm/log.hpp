#pragma once

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
