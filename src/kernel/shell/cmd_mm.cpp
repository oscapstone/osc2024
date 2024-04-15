#include "board/mini-uart.hpp"
#include "mm/heap.hpp"
#include "mm/page_alloc.hpp"
#include "shell/cmd.hpp"
#include "string.hpp"

int cmd_mm(int argc, char* argv[]) {
  if (argc <= 1) {
    heap_info();
    page_alloc.info();
  } else if (0 == strcmp(argv[1], "heap")) {
    if (argc <= 2) {
      heap_info();
    } else {
      for (int i = 2; i < argc; i++) {
        int size = strtol(argv[i]);
        auto addr = heap_malloc(size);
        mini_uart_printf("heap_malloc(%d) = %p\n", size, addr);
      }
    }
  } else if (0 == strcmp(argv[1], "page")) {
    if (argc <= 2) {
      page_alloc.info();
    } else {
      // TODO
    }
  } else {
    mini_uart_printf("mm: '%s' not found\n", argv[1]);
    return -1;
  }

  return 0;
}
