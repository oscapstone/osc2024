#include "mm/mm.hpp"

#include "fdt.hpp"
#include "int/interrupt.hpp"
#include "mm/heap.hpp"
#include "mm/new.hpp"
#include "mm/page.hpp"
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
  mm_page.preinit(start, end);
}

void mm_reserve_p(void* start, void* end) {
  mm_page.reserve(start, end);
}

void mm_init() {
  mm_reserve(__heap_start, __heap_end);

  mm_page.init();
  heap_init();
  set_new_delete_handler(kmalloc, kfree);
}

void* kmalloc(uint64_t size, uint64_t align) {
  void* res = nullptr;
  save_DAIF_disable_interrupt();
  // TODO: handle alignment
  if (size > max_chunk_size)
    res = mm_page.alloc(size);
  else
    res = heap_malloc(size);
  restore_DAIF();
  return res;
}

void kfree(void* ptr) {
  save_DAIF_disable_interrupt();
  if (isPageAlign(ptr))
    mm_page.free(ptr);
  else
    heap_free(ptr);
  restore_DAIF();
}
