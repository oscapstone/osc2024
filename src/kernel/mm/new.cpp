#include "mm/new.hpp"

#include "mm/startup.hpp"

void* operator new(unsigned long size) {
  return startup_malloc(size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}
void* operator new[](unsigned long size) {
  return startup_malloc(size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}
void operator delete(void* /*ptr*/) noexcept {
  // TODO
}
void operator delete[](void* /*ptr*/) noexcept {
  // TODO
}
