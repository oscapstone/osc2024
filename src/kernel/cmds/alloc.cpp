#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "heap.hpp"
#include "string.hpp"

int cmd_alloc(int argc, char* argv[]) {
  if (argc <= 1) {
    mini_uart_puts("alloc: require at least one argument\n");
    mini_uart_puts("usage: alloc <size ...>\n");
    return -1;
  }
  for (int i = 1; i < argc; i++) {
    int size = strtol(argv[i]);
    auto addr = heap_malloc(size);
    mini_uart_printf("heap_malloc(%d) = %p\n", size, addr);
  }
  return 0;
}
