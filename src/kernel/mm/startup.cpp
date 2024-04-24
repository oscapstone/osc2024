#include "mm/startup.hpp"

#include "io.hpp"
#include "mm/new.hpp"
#include "util.hpp"

extern char __heap_start[];
extern char __heap_end[];
char* heap_cur = __heap_start;

void startup_alloc_info() {
  kprintf("[startup alloc] usage %p / (%p ~ %p)\n", heap_cur, __heap_start,
          __heap_end);
}

void startup_alloc_init() {
  heap_cur = __heap_start;
  set_new_delete_handler(startup_malloc, startup_free);
}

void* startup_malloc(unsigned long size, unsigned long al) {
  heap_cur = align(heap_cur, al);
  if (!startup_free(size))
    return nullptr;
  void* tmp = heap_cur;
  heap_cur += size;
  return tmp;
}

void startup_free(void*) {}

bool startup_free(unsigned long size) {
  return heap_cur + size <= __heap_end;
}
