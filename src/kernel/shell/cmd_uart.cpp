#include "board/mini-uart.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_uart(int /*argc*/, char* /*argv*/[]) {
  mini_uart_use_async(!mini_uart_is_async);
  kprintf("use %s uart\n", mini_uart_is_async ? "async" : "sync");
  return 0;
}
