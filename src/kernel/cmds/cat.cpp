#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "fs/initramfs.hpp"

int cmd_cat(int argc, char* argv[]) {
  int r = 0;
  if (argc == 1) {
    for (;;) {
      char c = mini_uart_getc();
      if (c == 4)  // ^D
        break;
      mini_uart_putc(c);
    }
  } else {
    for (int i = 1; i < argc; i++) {
      auto name = argv[i];
      auto f = initramfs.find(name);
      if (f == nullptr) {
        r = -1;
        mini_uart_printf("cat: %s: No such file or directory\n", name);
      } else if (f->isdir()) {
        r = -1;
        mini_uart_printf("cat: %s: Is a directory\n", name);
      } else {
        for (auto c : f->file()) {
          mini_uart_putc(c);
        }
      }
    }
  }
  return r;
}
