#include "board/mini-uart.hpp"
#include "mm/heap.hpp"
#include "shell/cmd.hpp"
#include "string.hpp"

int cmd_heap(int argc, char* argv[]) {
  if (argc <= 1) {
    mini_uart_printf("heap %p / (%p ~ %p)\n", heap_cur, __heap_start,
                     __heap_end);
  } else {
    for (int i = 1; i < argc; i++) {
      int size = strtol(argv[i]);
      auto addr = heap_malloc(size);
      mini_uart_printf("heap_malloc(%d) = %p\n", size, addr);
    }
  }
  return 0;
}
