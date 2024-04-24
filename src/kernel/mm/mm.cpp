#include "mm/mm.hpp"

#include "fdt.hpp"
#include "mm/heap.hpp"
#include "mm/new.hpp"
#include "mm/page_alloc.hpp"
#include "mm/startup.hpp"
#include "pair.hpp"

pair<uint32_t, uint32_t> mm_range() {
  auto path = "/memory/reg";
  auto [found, view] = fdt.find(path);
  if (!found) {
    klog("mm: device %s not found\n", path);
    prog_hang();
  }
  auto value = fdt_ld64(view.data());
  uint32_t start = value >> 32;
  uint32_t end = value & MASK(32);
  return {start, end};
}

void mm_preinit() {
  startup_alloc_init();

  auto [start, end] = mm_range();
  page_alloc.preinit(start, end);
}

void mm_reserve_p(void* start, void* end) {
  page_alloc.reserve(start, end);
}

void mm_init() {
  mm_reserve(__heap_start, __heap_end);

  page_alloc.init();
  heap_init();
  set_new_delete_handler(kmalloc, kfree);
}

void* kmalloc(uint64_t size, uint64_t align) {
  if (size > max_chunk_size)
    return page_alloc.alloc(size);
  // TODO: handle alignment
  return heap_malloc(size);
}

void kfree(void* ptr) {
  if (isPageAlign(ptr))
    return page_alloc.free(ptr);
  heap_free(ptr);
}
