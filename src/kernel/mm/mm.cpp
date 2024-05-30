#include "mm/mm.hpp"

#include "fdt.hpp"
#include "int/interrupt.hpp"
#include "mm/heap.hpp"
#include "mm/mmu.hpp"
#include "mm/new.hpp"
#include "mm/page.hpp"
#include "mm/startup.hpp"

pair<uint64_t, uint64_t> mm_range() {
  auto path = "/memory/reg";
  auto [found, view] = fdt.find(path);
  if (not found)
    panic("mm: device %s not found", path);
  auto value = fdt_ld64(view.data());
  uint32_t start = value >> 32;
  uint32_t end = value & MASK(32);
  return {start, end};
}

void mm_preinit() {
  startup_alloc_init();

  auto [start, end] = mm_range();
  mm_page.preinit(pa2va(start), pa2va(end));
}

void mm_reserve_p(void* start, void* end) {
  mm_page.reserve(start, end);
}

void mm_init() {
  // startup allocator
  mm_reserve(__heap_start, __heap_end);

  mm_page.init();
  heap_init();
  set_new_delete_handler(kcalloc, kfree);
}

// TODO: handle alignment
void* kmalloc(uint64_t size, uint64_t align) {
  void* addr = nullptr;
  save_DAIF_disable_interrupt();
  if (size > max_chunk_size or align == PAGE_SIZE)
    addr = mm_page.alloc(size);
  else
    addr = heap_malloc(size);
  restore_DAIF();
  return addr;
}

void* kcalloc(uint64_t size, uint64_t align) {
  void* addr = kmalloc(size, align);
  if (addr)
    memset(addr, 0, size);
  return addr;
}

void kfree(void* ptr) {
  if (ptr == nullptr or is_startup(ptr))
    return;
  save_DAIF_disable_interrupt();
  if (isPageAlign(ptr))
    mm_page.free(ptr);
  else
    heap_free(ptr);
  restore_DAIF();
}
