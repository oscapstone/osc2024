#include "mm/page_alloc.hpp"

#include "board/mini-uart.hpp"
#include "math.hpp"
#include "util.hpp"

static_assert(sizeof(PageAlloc::FreePage) <= PAGE_SIZE);

#define LOG_LEVEL 3

#if LOG_LEVEL >= 3
#define DEBUG_PRINT(fmt, ...) mini_uart_printf(fmt __VA_OPT__(, ) __VA_ARGS__)
#define DEBUG(fmt, ...) \
  mini_uart_printf("[page_alloc] [DEBUG] " fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) 0
#define DEBUG(fmt, ...)       0
#endif

#if LOG_LEVEL >= 2
#define INFO(fmt, ...) \
  mini_uart_printf("[page_alloc] [*] " fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define INFO(fmt, ...) 0
#endif

void PageAlloc::info() {
  mini_uart_printf("== PageAlloc ==\n");
  for (uint64_t i = 0; i < length; i++) {
    if (array[i] != FRAME_NOT_HEAD)
      mini_uart_printf("  frame 0x%lx: %d\n", i, array[i]);
  }
  for (int8_t o = 0; o < total_order; o++)
    if (not free_list[o].empty()) {
      mini_uart_printf("  free_list %d: ", o);
      for (auto p : free_list[o])
        mini_uart_printf("%p -> ", p);
      mini_uart_printf("\n");
    }
  mini_uart_printf("---------------\n");
}

PageAlloc page_alloc;

void PageAlloc::init(uint64_t start_, uint64_t end_) {
  start = start_, end = end_;
  if (start % PAGE_SIZE or end % PAGE_SIZE) {
    mini_uart_printf(
        "%s: start 0x%lx or end 0x%lx not align with PAGE_SIZE 0x%lx\n",
        __func__, start, end, PAGE_SIZE);
    prog_hang();
  }

  length = (end - start) / PAGE_SIZE;
  total_order = log2c(length + 1);
  DEBUG("init: length = 0x%lx total_order = %x\n", length, total_order);
  array = new int8_t[length];
  memset(array, FRAME_NOT_HEAD, length);
  free_list = new ListHead<FreePage>[total_order];
  int8_t order = total_order - 1;
  for (uint64_t i = 0; i < length; i += (1ll << order)) {
    while (i + (1ll << order) > length)
      order--;
    newFreePage(i, order);
  }
#if LOG_LEVEL >= 3
  info();
#endif
}

PageAlloc::FreePage* PageAlloc::split(FreePage* page, int8_t order) {
  auto vpn = addr2vpn(page);
  auto ord = array[vpn];
  if (ord < order)
    return nullptr;
  while (ord > order) {
    ord--;
    array[vpn] = ord;
    DEBUG("split: frame %lu = order %d\n", buddy(vpn), ord);
    newFreePage(buddy(vpn), ord);
  }
  return page;
}

void* PageAlloc::alloc(uint64_t size) {
  int8_t order = log2c(size);
  DEBUG("alloc: size %lu -> order %d\n", size, order);
  for (int8_t ord = order; ord < total_order; ord++) {
    auto page = free_list[ord].pop_front();
    if (page == nullptr)
      continue;
    page = split(page, order);
    if (not page)
      continue;
    array[addr2vpn(page)] = FRAME_ALLOCATED;
    return page;
  }
  return nullptr;
}

void PageAlloc::free(void* ptr) {
  if (ptr == nullptr)
    return;

  // TODO
}
