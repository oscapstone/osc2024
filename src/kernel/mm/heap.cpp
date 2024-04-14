#include "mm/heap.hpp"

#include "util.hpp"

char* heap_cur = __heap_start;

void heap_reset() {
  heap_cur = __heap_start;
}

void* heap_malloc(int size, int al) {
  heap_cur = align(heap_cur, al);
  if (!heap_free(size))
    return nullptr;
  void* tmp = heap_cur;
  heap_cur += size;
  return tmp;
}

bool heap_free(int size) {
  return heap_cur + size <= __heap_end;
}

void* operator new(unsigned long size) {
  return heap_malloc(size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}
void* operator new[](unsigned long size) {
  return heap_malloc(size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}
void operator delete(void* /*ptr*/) noexcept {
  // TODO
}
void operator delete[](void* /*ptr*/) noexcept {
  // TODO
}
