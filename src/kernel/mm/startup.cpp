#include "mm/startup.hpp"

#include "board/mini-uart.hpp"
#include "util.hpp"

extern char __heap_start[];
extern char __heap_end[];
char* heap_cur = __heap_start;

void startup_alloc_info() {
  mini_uart_printf("heap %p / (%p ~ %p)\n", heap_cur, __heap_start, __heap_end);
}

void startup_alloc_reset() {
  heap_cur = __heap_start;
}

void* startup_malloc(int size, int al) {
  heap_cur = align(heap_cur, al);
  if (!startup_free(size))
    return nullptr;
  void* tmp = heap_cur;
  heap_cur += size;
  return tmp;
}

bool startup_free(int size) {
  return heap_cur + size <= __heap_end;
}
