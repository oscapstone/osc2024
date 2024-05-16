#include "io.hpp"
#include "mm/heap.hpp"
#include "mm/page.hpp"
#include "mm/startup.hpp"
#include "shell/cmd.hpp"
#include "string.hpp"

int cmd_mm(int argc, char* argv[]) {
  if (argc <= 1) {
    startup_alloc_info();
    mm_page.info();
    heap_info();
  } else if (0 == strcmp(argv[1], "startup")) {
    if (argc <= 2) {
      startup_alloc_info();
    } else {
      for (int i = 2; i < argc; i++) {
        int size = strtol(argv[i]);
        auto addr = startup_malloc(size);
        kprintf("startup_malloc(%d) = %p\n", size, addr);
      }
    }
  } else if (0 == strcmp(argv[1], "page")) {
    if (argc <= 2) {
      mm_page.info();
    } else if (0 == strcmp(argv[2], "alloc") and argc >= 4) {
      auto size = strtol(argv[3]);
      auto ptr = mm_page.alloc(size);
      kprintf("page: alloc(0x%lx) = %p\n", size, ptr);
    } else if (0 == strcmp(argv[2], "free") and argc >= 4) {
      auto ptr = (void*)strtol(argv[3]);
      kprintf("page: free(%p)\n", ptr);
      mm_page.free(ptr);
    } else {
      kprintf("mm: page action '%s' not match\n", argv[2]);
      return -1;
    }
  } else if (0 == strcmp(argv[1], "heap")) {
    if (argc <= 2) {
      heap_info();
    } else if (0 == strcmp(argv[2], "alloc") and argc >= 4) {
      auto size = strtol(argv[3]);
      auto ptr = heap_malloc(size);
      kprintf("heap_malloc(0x%lx) = %p\n", size, ptr);
    } else if (0 == strcmp(argv[2], "free") and argc >= 4) {
      auto ptr = (void*)strtol(argv[3]);
      kprintf("heap_free(%p)\n", ptr);
      heap_free(ptr);
    } else {
      kprintf("mm: heap action '%s' not match\n", argv[2]);
      return -1;
    }
  } else if (0 == strcmp(argv[1], "alloc") and argc >= 3) {
    auto size = strtol(argv[2]);
    auto ptr = kmalloc(size);
    kprintf("kmalloc(0x%lx) = %p\n", size, ptr);
  } else if (0 == strcmp(argv[1], "free") and argc >= 3) {
    auto ptr = (void*)strtol(argv[2]);
    kprintf("kfree(%p)\n", ptr);
    kfree(ptr);
  } else {
    kprintf("%s: '%s' not found\n", argv[0], argv[1]);
    return -1;
  }

  return 0;
}
