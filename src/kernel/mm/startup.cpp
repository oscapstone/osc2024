#include "mm/startup.hpp"

#include "io.hpp"
#include "mm/new.hpp"
#include "util.hpp"

char* heap_cur = __heap_start;

void startup_alloc_info() {
  kprintf("[startup alloc] usage %p / (%p ~ %p)\n", heap_cur, __heap_start,
          __heap_end);
}

void startup_alloc_init() {
  heap_cur = __heap_start;
  set_new_delete_handler(startup_malloc, startup_free);
}

void* startup_malloc(uint64_t size, uint64_t al) {
  heap_cur = align(heap_cur, al);
  if (!startup_free(size)) {
    klog("[startup alloc] oom require 0x%lx / %p / (%p ~ %p)\n", size, heap_cur,
         __heap_start, __heap_end);
    prog_hang();
    return nullptr;
  }
  void* tmp = heap_cur;
  heap_cur += size;
  return tmp;
}

void startup_free(void*) {}

bool startup_free(uint64_t size) {
  return heap_cur + size <= __heap_end;
}
