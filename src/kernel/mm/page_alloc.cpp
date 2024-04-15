#include "mm/page_alloc.hpp"

#include "std.hpp"
// std.hpp require before new
#include <new>

#include "board/mini-uart.hpp"
#include "math.hpp"
#include "util.hpp"

static_assert(sizeof(PageAlloc::Page) <= PAGE_SIZE);

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
  for (uint64_t i = 0; i < size; i++) {
    if (array[i] != FRAME_NOT_HEAD)
      mini_uart_printf("  frame 0x%lx: %d\n", i, array[i]);
  }
  for (int8_t o = 0; o < order; o++) {
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

  size = (end - start) / PAGE_SIZE;
  order = log2(size);
  DEBUG("size = 0x%lx order = %x\n", size, order);
  array = new int8_t[size];
  memset(array, FRAME_NOT_HEAD, size);
  free_list = new ListHead<Page>[order];
  int8_t ord = order - 1;
  for (uint64_t i = 0; i < size; i += (1ll << ord)) {
    while (i + (1ll << ord) > size)
      ord--;
    array[i] = ord;
    auto page = new (vpn2addr(i)) Page;
    free_list[ord].insert_tail(page);
  }
#if LOG_LEVEL >= 3
  info();
#endif
}
