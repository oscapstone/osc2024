#include "board/mini-uart.hpp"
#include "cmd.hpp"

void cmd_hello() {
  mini_uart_puts("Hello World!\n");
}
