#include "board/mini-uart.hpp"
#include "shell/cmd.hpp"

int cmd_uart(int /*argc*/, char* /*argv*/[]) {
  mini_uart_use_async(!mini_uart_is_async);
  mini_uart_printf("use %ssync uart\n", mini_uart_is_async ? "a" : "");
  return 0;
}
