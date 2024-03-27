#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "fdt.hpp"

int cmd_devtree(int argc, char* argv[]) {
  if (argc <= 1) {
    mini_uart_puts("devtree: require path\n");
    mini_uart_puts("usage: devtree <path> [depth]\n");
    return -1;
  }

  auto path = argv[1];
  int depth = argc >= 3 ? strtol(argv[2]) : 0;

  int r = 0;
  auto [found, view] = fdt.find(path, print_fdt, depth);
  if (not found) {
    r = -1;
    mini_uart_printf("devtree: %s: No such device\n", path);
  } else if (view.data()) {
    mini_uart_printf("%s: ", path);
    mini_uart_print(view);
    mini_uart_printf("\n");
  }
  return r;
}
