#include "board/timer.hpp"

#include "board/mini-uart.hpp"
#include "cmd.hpp"

int cmd_timer(int argc, char* argv[]) {
  if (argc <= 1) {
    show_timer = !show_timer;
  } else {
    auto c = argv[1][0];
    show_timer = c == 'y' or c == 't' or c == '1';
  }
  if (show_timer) {
    mini_uart_puts("show timer interrupt\n");
  } else {
    mini_uart_puts("hide timer interrupt\n");
  }
  return 0;
}
