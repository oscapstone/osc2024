#include "board/mini-uart.hpp"
#include "shell/cmd.hpp"

int cmd_hello(int /* argc */, char* /* argv */[]) {
  mini_uart_puts("Hello World!\n");
  return 0;
}
