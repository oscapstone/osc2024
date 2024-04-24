#include "mm/mm.hpp"

#include "mm/heap.hpp"
#include "mm/page_alloc.hpp"
#include "mm/startup.hpp"

void mm_init() {
  startup_alloc_init();
  page_alloc.init(0x1000'0000, 0x2000'0000);
  heap_init();
}
