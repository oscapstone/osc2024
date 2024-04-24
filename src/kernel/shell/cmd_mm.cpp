#include "io.hpp"
#include "mm/page_alloc.hpp"
#include "mm/startup.hpp"
#include "shell/cmd.hpp"
#include "string.hpp"

int cmd_mm(int argc, char* argv[]) {
  if (argc <= 1) {
    startup_alloc_info();
    page_alloc.info();
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
      page_alloc.info();
    } else if (0 == strcmp(argv[2], "alloc") and argc >= 4) {
      auto size = strtol(argv[3]);
      auto ptr = page_alloc.alloc(size);
      kprintf("page: alloc(0x%lx) = %p\n", size, ptr);
      page_alloc.info();
    } else if (0 == strcmp(argv[2], "free") and argc >= 4) {
      auto ptr = (void*)strtol(argv[3]);
      kprintf("page: free(%p)\n", ptr);
      page_alloc.free(ptr);
      page_alloc.info();
    } else {
      kprintf("mm: page action '%s' not match\n", argv[2]);
      return -1;
    }
  } else {
    kprintf("%s: '%s' not found\n", argv[0], argv[1]);
    return -1;
  }

  return 0;
}
