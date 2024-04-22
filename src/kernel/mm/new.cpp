#include "mm/new.hpp"

#include "mm/startup.hpp"

void* default_malloc(unsigned long, unsigned long) {
  return nullptr;
}
void default_free(void*) {}

malloc_fp malloc_handler = default_malloc;
free_fp free_handler = default_free;

void set_new_delete_handler(malloc_fp malloc_, free_fp free_) {
  malloc_handler = malloc_;
  free_handler = free_;
}

void* operator new(unsigned long size) {
  return malloc_handler(size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}
void* operator new[](unsigned long size) {
  return startup_malloc(size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}
void operator delete(void* ptr) noexcept {
  free_handler(ptr);
}
void operator delete[](void* ptr) noexcept {
  free_handler(ptr);
}
