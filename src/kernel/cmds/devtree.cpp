#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "fdt.hpp"

int cmd_devtree(int argc, char* argv[]) {
  int r = 0;
  if (argc <= 1) {
    fdt.print();
  } else {
    for (int i = 1; i < argc; i++) {
      auto name = argv[i];
      auto view = fdt.find(name);
      if (view.data() == nullptr) {
        r = -1;
        mini_uart_printf("devtree: %s: No such device\n", name);
      } else {
        mini_uart_printf("%s: ", name);
        mini_uart_print(view);
        mini_uart_printf("\n");
      }
    }
  }
  return r;
}
