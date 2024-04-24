#include "mm/mm.hpp"

#include "mm/heap.hpp"
#include "mm/new.hpp"
#include "mm/page_alloc.hpp"
#include "mm/startup.hpp"

void mm_init() {
  startup_alloc_init();
  page_alloc.init(0x1000'0000, 0x2000'0000);
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
