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

PageAlloc page_alloc;

void PageAlloc::info() {
  mini_uart_printf("== PageAlloc ==\n");
  for (uint64_t i = 0; i < length; i++) {
    if (array[i].head())
      mini_uart_printf("  frame 0x%lx: %d %s\n", i, array[i].order,
                       array[i].allocated ? "allocated" : "free");
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
  array = new Frame[length];

  for (uint64_t i = 0; i < length; i++)
    array[i].value = FRAME_NOT_HEAD;
  free_list = new ListHead<FreePage>[total_order];
  int8_t order = total_order - 1;
  for (uint64_t i = 0; i < length; i += (1ll << order)) {
    while (i + (1ll << order) > length)
      order--;
    array[i].order = order;
    auto page = new (vpn2addr(i)) FreePage;
    free_list[order].insert_tail(page);
  }
#if LOG_LEVEL >= 3
  info();
#endif
}

PageAlloc::FreePage* PageAlloc::release(PageAlloc::AllocatedPage apage) {
  auto vpn = addr2vpn(apage);
  auto order = array[vpn].order;
  INFO("release: page %p order %d\n", apage, order);
  array[vpn].allocated = false;
  auto fpage = new (vpn2addr(vpn)) FreePage;
  free_list[order].insert_tail(fpage);
  return fpage;
}

PageAlloc::AllocatedPage PageAlloc::alloc(PageAlloc::FreePage* fpage) {
  auto vpn = addr2vpn(fpage);
  auto order = array[vpn].order;
  INFO("alloc: page %p order %d\n", fpage, order);
  array[vpn].allocated = true;
  free_list[order].erase(fpage);
  auto apage = AllocatedPage(fpage);
  return apage;
}

PageAlloc::AllocatedPage PageAlloc::split(AllocatedPage apage) {
  auto vpn = addr2vpn(apage);
  auto o = --array[vpn].order;
  auto bvpn = buddy(vpn);
  array[bvpn] = {.allocated = true, .order = o};
  auto bpage = vpn2addr(bvpn);
  DEBUG("split: %p + %p\n", apage, bpage);
  return bpage;
}

void PageAlloc::truncate(AllocatedPage apage, int8_t order) {
  auto vpn = addr2vpn(apage);
  while (array[vpn].order > order) {
    auto bpage = split(apage);
    release(bpage);
  }
}

void* PageAlloc::alloc(uint64_t size) {
  int8_t order = log2c(size);
  DEBUG("alloc: size %lu -> order %d\n", size, order);
  for (int8_t ord = order; ord < total_order; ord++) {
    auto fpage = free_list[ord].front();
    if (fpage == nullptr)
      continue;
    auto apage = alloc(fpage);
    truncate(apage, order);
    DEBUG("alloc: -> page %p\n", apage);
    return apage;
  }
  DEBUG("alloc: no page availiable\n");
  return nullptr;
}

void PageAlloc::free(void* ptr) {
  if (ptr == nullptr)
    return;

  // TODO
}
