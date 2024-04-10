#include "timer.hpp"

#include "board/mini-uart.hpp"
#include "cmd.hpp"

int cmd_timer(int argc, char* argv[]) {
  if (argc <= 1) {
    show_timer = !show_timer;
  } else {
    auto c = argv[1][0];
    show_timer = c == 'y' or c == 't' or c == '1';
  }
  mini_uart_printf("%s timer interrupt\n", show_timer ? "show" : "hide");
  return 0;
}
